/**
 * @file network_backend.cpp
 * @brief Userspace NAT network backend (libslirp)
 */

#include <o2emu/logging/logger.h>
#include <o2emu/system/network_backend.h>
#include <o2emu/system/network_config.h>
#include <o2emu/system/nfs_server.h>

#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <vector>

#ifdef O2EMU_HAVE_SLIRP
#include <poll.h>
#include <slirp/libslirp.h>
#endif

namespace o2emu::system {

namespace {

bool parse_addr(const std::string &text, in_addr &out) {
  return inet_pton(AF_INET, text.c_str(), &out) == 1;
}

} // namespace

#ifdef O2EMU_HAVE_SLIRP

struct NetworkBackend::Impl {
  Slirp *slirp = nullptr;

  // State used while collecting pollfds inside slirp_pollfds_fill().
  std::vector<pollfd> fds;
};

static ssize_t slirp_output_cb(const void *buf, size_t len, void *opaque) {
  auto *self = static_cast<NetworkBackend *>(opaque);
  self->deliver_rx(static_cast<const uint8_t *>(buf), len);
  return static_cast<ssize_t>(len);
}

static int slirp_add_poll_cb(int fd, int events, void *opaque) {
  auto *fds = static_cast<std::vector<pollfd> *>(opaque);
  fds->push_back({fd, static_cast<short>(events), 0});
  return static_cast<int>(fds->size()) - 1;
}

static int slirp_get_revents_cb(int idx, void *opaque) {
  auto *fds = static_cast<std::vector<pollfd> *>(opaque);
  if (idx < 0 || static_cast<size_t>(idx) >= fds->size())
    return 0;
  return (*fds)[idx].revents;
}

#else // !O2EMU_HAVE_SLIRP

struct NetworkBackend::Impl {};

#endif // O2EMU_HAVE_SLIRP

NetworkBackend::NetworkBackend() = default;

NetworkBackend::~NetworkBackend() { stop(); }

bool NetworkBackend::available() {
#ifdef O2EMU_HAVE_SLIRP
  return true;
#else
  return false;
#endif
}

bool NetworkBackend::start(const NetworkConfig &config) {
  stop();

#ifndef O2EMU_HAVE_SLIRP
  last_error_ = "O2Emu was built without libslirp; NAT networking is "
                "unavailable. Install libslirp and rebuild.";
  O2EMU_LOG_ERROR("{}", last_error_);
  return false;
#else
  impl_ = std::make_unique<Impl>();

  SlirpConfig cfg{};
  cfg.version = 1; // legacy timer model: timeouts via slirp_pollfds_fill()
  cfg.restricted = 0;
  cfg.in_enabled = 1;
  cfg.if_mtu = 1500;
  cfg.if_mru = 1500;
  cfg.disable_host_loopback = 0;
  cfg.enable_emu = 0;

  if (!parse_addr(config.network(), cfg.vnetwork) ||
      !parse_addr(config.netmask(), cfg.vnetmask) ||
      !parse_addr(config.gateway(), cfg.vhost) ||
      !parse_addr(config.guest_address(), cfg.vdhcp_start)) {
    last_error_ = "Invalid network configuration addresses.";
    return false;
  }

  // Built-in DNS lives at the classic slirp address (network + 3).
  uint32_t dns = ntohl(cfg.vnetwork.s_addr) + 3;
  cfg.vnameserver.s_addr = htonl(dns);

  SlirpCb cb{};
  cb.output = &slirp_output_cb;

  impl_->slirp = slirp_new(&cfg, &cb, this);
  if (!impl_->slirp) {
    last_error_ = "Failed to create the NAT backend (slirp_new failed).";
    impl_.reset();
    return false;
  }

  // Port forwarding: host loopback port -> guest port.
  in_addr host_addr{};
  host_addr.s_addr = htonl(INADDR_LOOPBACK);
  in_addr guest_addr = cfg.vdhcp_start;
  for (const auto &forward : config.port_forwards()) {
    if (!forward.enabled || forward.host_port == 0 || forward.guest_port == 0)
      continue;
    const int is_udp = forward.protocol == "UDP";
    if (slirp_add_hostfwd(impl_->slirp, is_udp, host_addr, forward.host_port,
                          guest_addr, forward.guest_port) < 0) {
      O2EMU_LOG_WARN("Failed to forward {} port {} -> guest {}",
                     forward.protocol, forward.host_port, forward.guest_port);
    } else {
      O2EMU_LOG_INFO("Forwarding {} localhost:{} -> guest:{}", forward.protocol,
                     forward.host_port, forward.guest_port);
    }
  }

  // NFS share: serve the configured folder on the host loopback so the
  // guest can reach it at the gateway address.
  if (!config.share_folder().empty()) {
    nfs_ = std::make_unique<NfsServer>();
    if (!nfs_->start(config.share_folder())) {
      O2EMU_LOG_WARN("NFS share failed to start: {}", nfs_->last_error());
      nfs_.reset();
    } else {
      O2EMU_LOG_INFO("NFS share '{}' available at {} (mount -o "
                     "port=2049,mountport=2048 {}:/ /mnt)",
                     config.share_folder(), config.gateway(), config.gateway());
    }
  }

  running_ = true;
  last_error_.clear();
  O2EMU_LOG_INFO("NAT network {} / {} up, gateway {}, guest {}",
                 config.network(), config.netmask(), config.gateway(),
                 config.guest_address());
  return true;
#endif
}

void NetworkBackend::stop() {
  if (nfs_) {
    nfs_->stop();
    nfs_.reset();
  }
#ifdef O2EMU_HAVE_SLIRP
  if (impl_ && impl_->slirp) {
    slirp_cleanup(impl_->slirp);
    impl_->slirp = nullptr;
  }
  impl_.reset();
#endif
  running_ = false;
}

void NetworkBackend::send_frame(const uint8_t *data, size_t length) {
#ifdef O2EMU_HAVE_SLIRP
  if (!running_ || !impl_ || !impl_->slirp || !data || length == 0)
    return;
  slirp_input(impl_->slirp, data, static_cast<int>(length));
#else
  (void)data;
  (void)length;
#endif
}

void NetworkBackend::set_rx_handler(
    std::function<void(const uint8_t *, size_t)> handler) {
  rx_handler_ = std::move(handler);
}

void NetworkBackend::deliver_rx(const uint8_t *data, size_t length) {
  if (rx_handler_)
    rx_handler_(data, length);
}

void NetworkBackend::poll() {
#ifdef O2EMU_HAVE_SLIRP
  if (!running_ || !impl_ || !impl_->slirp)
    return;

  // slirp's legacy timer model needs a poll at least every few hundred ms;
  // tick() calls us far more often, so cap the wait at 10 ms.
  uint32_t timeout = 10;
  impl_->fds.clear();
  slirp_pollfds_fill(impl_->slirp, &timeout, &slirp_add_poll_cb, &impl_->fds);

  if (!impl_->fds.empty())
    ::poll(impl_->fds.data(), impl_->fds.size(), 0);

  const int error = 0;
  slirp_pollfds_poll(impl_->slirp, error, &slirp_get_revents_cb, &impl_->fds);
#endif
}

} // namespace o2emu::system
