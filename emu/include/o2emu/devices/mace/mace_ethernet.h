#pragma once

/**
 * @file mace_ethernet.h
 * @brief MACE Ethernet (MAC110 core)
 *
 * Offset from MACE base: 0x280000
 * Based on NetBSD sys/arch/sgimips/mace/if_mecreg.h
 * and Linux drivers/net/ethernet/sgi/meth.h
 * All registers are 32-bit, accessed at 4-byte offsets
 */

#include <array>
#include <o2emu/devices/device.h>
#include <o2emu/o2emu.h>

namespace o2emu::devices {

class MACE;

class MACEEthernet {
public:
  explicit MACEEthernet(MACE &mace);
  ~MACEEthernet();

  // MAC110 register offsets (from MACE base + 0x280000)
  // All registers are 32-bit, accessed at 4-byte offsets
  enum Register : uint32_t {
    REG_CTRL = 0x0000,
    REG_STATUS = 0x0008,
    REG_MAC_ADDR0 = 0x0010,
    REG_MAC_ADDR1 = 0x0018,
    REG_MCAST_FILTER = 0x0020,
    REG_TX_BASE = 0x0100,
    REG_TX_PTR = 0x0108,
    REG_TX_END = 0x0110,
    REG_TX_CTRL = 0x0118,
    REG_RX_BASE = 0x0200,
    REG_RX_PTR = 0x0208,
    REG_RX_END = 0x0210,
    REG_RX_CTRL = 0x0218,
    REG_INT_STATUS = 0x0300,
    REG_INT_MASK = 0x0308,
    REG_INT_CLEAR = 0x0310,
    REG_STATS_BASE = 0x0500,
  };

  // REG_CTRL bits
  enum CtrlBit : uint32_t {
    CTRL_RX_EN = 0x00000001,
    CTRL_TX_EN = 0x00000002,
    CTRL_LOOPBACK = 0x00000004,
    CTRL_PROMISC = 0x00000008,
    CTRL_ALL_MULTI = 0x00000010,
    CTRL_FULL_DUPLEX = 0x00000020,
    CTRL_100MBPS = 0x00000040,
    CTRL_PAD_EN = 0x00000080,
    CTRL_CRC_EN = 0x00000100,
    CTRL_HUGE_EN = 0x00000200,
  };

  // REG_STATUS bits
  enum StatusBit : uint32_t {
    STATUS_LINK_UP = 0x00000001,
    STATUS_FULL_DUPLEX = 0x00000002,
    STATUS_100MBPS = 0x00000004,
    STATUS_TX_ACTIVE = 0x00000008,
    STATUS_RX_ACTIVE = 0x00000010,
  };

  // REG_TX_CTRL bits
  enum TxCtrlBit : uint32_t {
    TX_CTRL_EN = 0x00000001,
    TX_CTRL_START = 0x00000002,
  };

  // REG_RX_CTRL bits
  enum RxCtrlBit : uint32_t {
    RX_CTRL_EN = 0x00000001,
    RX_CTRL_START = 0x00000002,
  };

  // Interrupt bits
  enum IntBit : uint32_t {
    INT_TX_DONE = 0x00000001,
    INT_TX_ERROR = 0x00000002,
    INT_RX_DONE = 0x00000004,
    INT_RX_ERROR = 0x00000008,
    INT_RX_OVERRUN = 0x00000010,
    INT_MDIO_DONE = 0x00000020,
    INT_LINK_CHANGE = 0x00000040,
  };

  // Register access
  u32 read_reg(u32 offset);
  void write_reg(u32 offset, u32 value);

  // Internal methods
  void handle_ctrl_write(u32 value);
  void update_mac_addr();
  void start_tx();
  void start_rx();

  // Device interface
  bool read(u32 offset, u32 size, u32 &value);
  bool write(u32 offset, u32 size, u32 value);

  void tick(u64 cycles);
  u32 interrupt_status() const;

  void reset();

  // MAC address
  void set_mac_address(const u8 addr[6]);
  void get_mac_address(u8 addr[6]) const;

private:
  MACE &mace_;
  std::array<u32, 0x1000 / 4> regs_ = {};

  // MAC address
  u8 mac_addr_[6] = {0x08, 0x00, 0x69, 0x00, 0x00, 0x00}; // SGI OUI

  // Link status
  bool link_up_ = false;
  bool full_duplex_ = true;
  bool speed_100mbps_ = true;
};

} // namespace o2emu::devices