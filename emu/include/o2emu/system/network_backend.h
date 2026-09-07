#pragma once

/**
 * @file network_backend.h
 * @brief Userspace NAT network backend (libslirp)
 *
 * Provides the emulated O2 with outbound-only Internet access through a
 * private NAT network inside the emulator. Nothing on the host network can
 * see the guest except through explicitly configured port forwards.
 *
 * Uses libslirp (QEMU's userspace TCP/IP stack) when available. When built
 * without libslirp, start() fails gracefully and the link stays down.
 */

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace o2emu::system {

class NetworkConfig;
class NfsServer;

class NetworkBackend {
public:
  NetworkBackend();
  ~NetworkBackend();

  NetworkBackend(const NetworkBackend &) = delete;
  NetworkBackend &operator=(const NetworkBackend &) = delete;

  /// True when built with libslirp support.
  static bool available();

  /// Start the NAT backend from the given configuration. Returns false on
  /// failure (e.g. built without libslirp, or invalid addresses).
  bool start(const NetworkConfig &config);
  void stop();
  bool running() const { return running_; }

  /// Inject a raw Ethernet frame transmitted by the guest.
  void send_frame(const uint8_t *data, size_t length);

  /// Handler invoked (on the emulation thread) with frames for the guest.
  void set_rx_handler(std::function<void(const uint8_t *, size_t)> handler);
  /// Poll host sockets and timers. Must be called regularly from the
  /// emulation thread (e.g. from the Ethernet device's tick()).
  void poll();

  /// Last error message from start().
  const std::string &last_error() const { return last_error_; }

  /// Internal: called by the slirp output callback with frames for the guest.
  void deliver_rx(const uint8_t *data, size_t length);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  std::unique_ptr<NfsServer> nfs_;
  std::function<void(const uint8_t *, size_t)> rx_handler_;
  bool running_ = false;
  std::string last_error_;
};

} // namespace o2emu::system
