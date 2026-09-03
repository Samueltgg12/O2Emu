/**
 * @file mace_ethernet.cpp
 * @brief MACE Ethernet (MACE MAC) implementation
 */

#include <cstring>
#include <o2emu/devices/mace/mace_ethernet.h>
#include <o2emu/logging/logger.h>

namespace o2emu::devices {

MACEEthernet::MACEEthernet(MACE &mace) : mace_(mace) { reset(); }

MACEEthernet::~MACEEthernet() = default;

void MACEEthernet::reset() {
  std::memset(regs_, 0, sizeof(regs_));
  std::memset(mac_addr_, 0, sizeof(mac_addr_));

  // Default MAC address (SGI OUI: 00:00:5E)
  mac_addr_[0] = 0x00;
  mac_addr_[1] = 0x00;
  mac_addr_[2] = 0x5E;
  mac_addr_[3] = 0x00;
  mac_addr_[4] = 0x00;
  mac_addr_[5] = 0x01;

  // MACE Ethernet register defaults (based on MACE/MACE2 specs)
  // MAC_CTRL: 0x0000
  regs_[REG_CTRL] = 0x00000000;

  // MAC_STATUS: 0x0008
  regs_[REG_STATUS] = 0x00000000;

  // MAC_MAC_ADDR0: 0x0010
  regs_[REG_MAC_ADDR0] = (mac_addr_[0] << 24) | (mac_addr_[1] << 16) |
                         (mac_addr_[2] << 8) | mac_addr_[3];

  // MAC_MAC_ADDR1: 0x0018
  regs_[REG_MAC_ADDR1] = (mac_addr_[4] << 24) | (mac_addr_[5] << 16);

  // MAC_MCAST_FILTER: 0x0020
  regs_[REG_MCAST_FILTER] = 0x00000000;

  // MAC_TX_BASE: 0x0100
  regs_[REG_TX_BASE] = 0x00000000;

  // MAC_TX_PTR: 0x0108
  regs_[REG_TX_PTR] = 0x00000000;

  // MAC_TX_END: 0x0110
  regs_[REG_TX_END] = 0x00000000;

  // MAC_TX_CTRL: 0x0118
  regs_[REG_TX_CTRL] = 0x00000000;

  // MAC_RX_BASE: 0x0200
  regs_[REG_RX_BASE] = 0x00000000;

  // MAC_RX_PTR: 0x0208
  regs_[REG_RX_PTR] = 0x00000000;

  // MAC_RX_END: 0x0210
  regs_[REG_RX_END] = 0x00000000;

  // MAC_RX_CTRL: 0x0218
  regs_[REG_RX_CTRL] = 0x00000000;

  // MAC_INT_STATUS: 0x0300
  regs_[REG_INT_STATUS] = 0x00000000;

  // MAC_INT_MASK: 0x0308
  regs_[REG_INT_MASK] = 0x00000000;

  // MAC_INT_CLEAR: 0x0310
  regs_[REG_INT_CLEAR] = 0x00000000;

  // Statistics counters
  for (int i = 0; i < 16; ++i) {
    regs_[REG_STATS_BASE + i] = 0;
  }
}

u32 MACEEthernet::read_reg(u32 offset) {
  if (offset < sizeof(regs_)) {
    return regs_[offset / 4];
  }
  return 0;
}

void MACEEthernet::write_reg(u32 offset, u32 value) {
  if (offset >= sizeof(regs_))
    return;

  u32 reg = offset / 4;

  switch (reg) {
  case REG_CTRL:
    regs_[reg] = value;
    handle_ctrl_write(value);
    break;

  case REG_MAC_ADDR0:
  case REG_MAC_ADDR1:
    regs_[reg] = value;
    update_mac_addr();
    break;

  case REG_MCAST_FILTER:
    regs_[reg] = value;
    break;

  case REG_TX_BASE:
  case REG_TX_PTR:
  case REG_TX_END:
  case REG_TX_CTRL:
    regs_[reg] = value;
    if (reg == REG_TX_CTRL && (value & 0x1)) {
      start_tx();
    }
    break;

  case REG_RX_BASE:
  case REG_RX_PTR:
  case REG_RX_END:
  case REG_RX_CTRL:
    regs_[reg] = value;
    if (reg == REG_RX_CTRL && (value & 0x1)) {
      start_rx();
    }
    break;

  case REG_INT_MASK:
    regs_[reg] = value;
    break;

  case REG_INT_CLEAR:
    regs_[REG_INT_STATUS] &= ~value;
    break;

  default:
    regs_[reg] = value;
    break;
  }
}

void MACEEthernet::handle_ctrl_write(u32 value) {
  // Bit 0: Enable
  // Bit 1: Loopback
  // Bit 2: Promiscuous mode
  // Bit 3: Full duplex
  // Bit 4: 100Mbps (vs 10Mbps)
  // Bit 5: Auto-negotiation
  // Bit 6: Reset
  // Bit 7: Statistics enable

  if (value & 0x40) { // Reset
    reset();
  }

  O2EMU_LOG_DEBUG("MAC_CTRL write: 0x" << std::hex << value << std::dec);
}

void MACEEthernet::update_mac_addr() {
  u32 addr0 = regs_[REG_MAC_ADDR0];
  u32 addr1 = regs_[REG_MAC_ADDR1];

  mac_addr_[0] = (addr0 >> 24) & 0xFF;
  mac_addr_[1] = (addr0 >> 16) & 0xFF;
  mac_addr_[2] = (addr0 >> 8) & 0xFF;
  mac_addr_[3] = addr0 & 0xFF;
  mac_addr_[4] = (addr1 >> 24) & 0xFF;
  mac_addr_[5] = (addr1 >> 16) & 0xFF;
}

void MACEEthernet::start_tx() {
  u32 tx_base = regs_[REG_TX_BASE];
  u32 tx_ptr = regs_[REG_TX_PTR];
  u32 tx_end = regs_[REG_TX_END];

  O2EMU_LOG_DEBUG("Ethernet TX start: base=0x"
                  << std::hex << tx_base << " ptr=0x" << tx_ptr << " end=0x"
                  << tx_end << std::dec);

  // Simulate TX completion
  regs_[REG_TX_PTR] = tx_end;
  regs_[REG_INT_STATUS] |= 0x1; // TX complete
}

void MACEEthernet::start_rx() {
  u32 rx_base = regs_[REG_RX_BASE];
  u32 rx_ptr = regs_[REG_RX_PTR];
  u32 rx_end = regs_[REG_RX_END];

  O2EMU_LOG_DEBUG("Ethernet RX start: base=0x"
                  << std::hex << rx_base << " ptr=0x" << rx_ptr << " end=0x"
                  << rx_end << std::dec);

  // Simulate RX - in real implementation, this would wait for packets
  regs_[REG_INT_STATUS] |= 0x2; // RX complete
}

bool MACEEthernet::read(u32 offset, u32 size, u32 &value) {
  if (offset < sizeof(regs_)) {
    value = read_reg(offset);
    return true;
  }
  return false;
}

bool MACEEthernet::write(u32 offset, u32 size, u32 value) {
  if (offset < sizeof(regs_)) {
    write_reg(offset, value);
    return true;
  }
  return false;
}

void MACEEthernet::tick(u64 cycles) {
  // Ethernet doesn't need periodic updates in this simple implementation
}

u32 MACEEthernet::interrupt_status() const {
  return regs_[REG_INT_STATUS] & regs_[REG_INT_MASK];
}

} // namespace o2emu::devices