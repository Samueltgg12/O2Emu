#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace o2emu::system {

struct PortForward {
  std::string protocol = "TCP";
  uint16_t host_port = 0;
  uint16_t guest_port = 0;
  bool enabled = true;
};

class NetworkConfig {
public:
  NetworkConfig();

  bool enabled() const { return enabled_; }
  void set_enabled(bool enabled) { enabled_ = enabled; }

  const std::string &network() const { return network_; }
  bool set_network(const std::string &network);
  const std::string &netmask() const { return netmask_; }
  bool set_netmask(const std::string &netmask);

  std::string gateway() const;
  std::string guest_address() const;
  bool conflicts_with_host() const;
  std::string conflict_description() const;

  const std::vector<PortForward> &port_forwards() const { return forwards_; }
  std::vector<PortForward> &port_forwards() { return forwards_; }

  const std::string &share_folder() const { return share_folder_; }
  void set_share_folder(std::string folder) {
    share_folder_ = std::move(folder);
  }

private:
  bool enabled_ = false;
  std::string network_ = "192.168.127.0";
  std::string netmask_ = "255.255.255.0";
  std::vector<PortForward> forwards_;
  std::string share_folder_;

  static bool parse_ipv4(const std::string &text, uint32_t &address);
  static std::string format_ipv4(uint32_t address);
  static std::vector<std::pair<uint32_t, uint32_t>> host_networks();
};

} // namespace o2emu::system
