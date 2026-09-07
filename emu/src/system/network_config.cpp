#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <o2emu/system/network_config.h>
#include <sstream>

namespace o2emu::system {

NetworkConfig::NetworkConfig() {
  forwards_ = {{"TCP", 2323, 23, true},
               {"TCP", 2222, 22, false},
               {"TCP", 2121, 21, false},
               {"TCP", 5900, 5900, false},
               {"TCP", 177, 177, false}};
}

bool NetworkConfig::parse_ipv4(const std::string &text, uint32_t &address) {
  in_addr parsed{};
  if (inet_pton(AF_INET, text.c_str(), &parsed) != 1)
    return false;
  address = ntohl(parsed.s_addr);
  return true;
}

std::string NetworkConfig::format_ipv4(uint32_t address) {
  std::ostringstream result;
  result << ((address >> 24) & 0xff) << '.' << ((address >> 16) & 0xff) << '.'
         << ((address >> 8) & 0xff) << '.' << (address & 0xff);
  return result.str();
}

bool NetworkConfig::set_network(const std::string &network) {
  uint32_t address = 0;
  if (!parse_ipv4(network, address))
    return false;
  const uint32_t first = (address >> 24) & 0xff;
  const uint32_t second = (address >> 16) & 0xff;
  const bool private_range = first == 10 ||
                             (first == 172 && second >= 16 && second <= 31) ||
                             (first == 192 && second == 168);
  if (!private_range)
    return false;
  network_ = network;
  return true;
}

bool NetworkConfig::set_netmask(const std::string &netmask) {
  uint32_t mask = 0;
  if (!parse_ipv4(netmask, mask) || mask == 0)
    return false;
  if ((mask | (mask - 1)) != 0xffffffffu)
    return false;
  netmask_ = netmask;
  return true;
}

std::string NetworkConfig::gateway() const {
  uint32_t network = 0;
  parse_ipv4(network_, network);
  return format_ipv4(network + 1);
}

std::string NetworkConfig::guest_address() const {
  uint32_t network = 0;
  parse_ipv4(network_, network);
  return format_ipv4(network + 2);
}

std::vector<std::pair<uint32_t, uint32_t>> NetworkConfig::host_networks() {
  std::vector<std::pair<uint32_t, uint32_t>> result;
  ifaddrs *interfaces = nullptr;
  if (getifaddrs(&interfaces) != 0)
    return result;

  for (ifaddrs *entry = interfaces; entry; entry = entry->ifa_next) {
    if (!entry->ifa_addr || !entry->ifa_netmask ||
        entry->ifa_addr->sa_family != AF_INET)
      continue;
    auto *address = reinterpret_cast<sockaddr_in *>(entry->ifa_addr);
    auto *mask = reinterpret_cast<sockaddr_in *>(entry->ifa_netmask);
    result.emplace_back(ntohl(address->sin_addr.s_addr) &
                            ntohl(mask->sin_addr.s_addr),
                        ntohl(mask->sin_addr.s_addr));
  }
  freeifaddrs(interfaces);
  return result;
}

bool NetworkConfig::conflicts_with_host() const {
  uint32_t network = 0;
  uint32_t mask = 0;
  if (!parse_ipv4(network_, network) || !parse_ipv4(netmask_, mask))
    return true;
  network &= mask;
  for (const auto &[host_network, host_mask] : host_networks()) {
    const uint32_t common_mask = mask & host_mask;
    if ((network & common_mask) == (host_network & common_mask))
      return true;
  }
  return false;
}

std::string NetworkConfig::conflict_description() const {
  return conflicts_with_host()
             ? "Conflicts with a host interface or route. Choose another "
               "private subnet."
             : "No conflict detected with the host interfaces.";
}

} // namespace o2emu::system
