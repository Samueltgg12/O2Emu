#pragma once

/**
 * @file mace_ethernet.h
 * @brief MACE Ethernet (MAC110 core)
 *
 * Offset from MACE base: 0x280000
 * Based on NetBSD sys/arch/sgimips/mace/if_mecreg.h
 * and Linux drivers/net/ethernet/sgi/meth.h
 * All registers are 64-bit, accessed at 8-byte offsets
 */

#include <array>
#include <o2emu/devices/device.h>
#include <o2emu/o2emu.h>
#include <vector>

namespace o2emu::devices {

class MACEEthernet : public Device {
public:
  MACEEthernet();
  ~MACEEthernet() override;

  // MAC110 register offsets (from MACE base + 0x280000)
  // All registers are 64-bit, 8-byte aligned
  enum Register : uint32_t {
    // MAC Control
    MAC_CTRL = 0x0000,
    MAC_ADDR_HIGH = 0x0008,
    MAC_ADDR_LOW = 0x0010,
    MAC_HASH_HIGH = 0x0018,
    MAC_HASH_LOW = 0x0020,

    // Transmit
    TX_CTRL = 0x0100,
    TX_STATUS = 0x0108,
    TX_DESC_BASE = 0x0110,
    TX_DESC_CUR = 0x0118,
    TX_DESC_END = 0x0120,

    // Receive
    RX_CTRL = 0x0200,
    RX_STATUS = 0x0208,
    RX_DESC_BASE = 0x0210,
    RX_DESC_CUR = 0x0218,
    RX_DESC_END = 0x0220,

    // Interrupt
    INT_STATUS = 0x0300,
    INT_MASK = 0x0308,
    INT_ACK = 0x0310,

    // PHY/MDIO
    MDIO_CTRL = 0x0400,
    MDIO_DATA = 0x0408,

    // Statistics
    STAT_RX_OK = 0x0500,
    STAT_RX_CRC_ERR = 0x0508,
    STAT_RX_ALIGN_ERR = 0x0510,
    STAT_RX_OVERRUN = 0x0518,
    STAT_TX_OK = 0x0520,
    STAT_TX_COLLISIONS = 0x0528,
    STAT_TX_DEFERRED = 0x0530,
    STAT_TX_LATE_COLL = 0x0538,
    STAT_TX_CARRIER_LOST = 0x0540,

    // Revision
    REVISION = 0x0FF8,
  };

  // MAC_CTRL bits
  enum MacCtrlBit : uint64_t {
    MAC_CTRL_RX_EN = 0x00000001,
    MAC_CTRL_TX_EN = 0x00000002,
    MAC_CTRL_LOOPBACK = 0x00000004,
    MAC_CTRL_PROMISC = 0x00000008,
    MAC_CTRL_ALL_MULTI = 0x00000010,
    MAC_CTRL_FULL_DUPLEX = 0x00000020,
    MAC_CTRL_100MBPS = 0x00000040,
    MAC_CTRL_PAD_EN = 0x00000080,
    MAC_CTRL_CRC_EN = 0x00000100,
    MAC_CTRL_HUGE_EN = 0x00000200,
  };

  // TX_CTRL bits
  enum TxCtrlBit : uint64_t {
    TX_CTRL_EN = 0x00000001,
    TX_CTRL_START = 0x00000002,
  };

  // RX_CTRL bits
  enum RxCtrlBit : uint64_t {
    RX_CTRL_EN = 0x00000001,
    RX_CTRL_START = 0x00000002,
  };

  // Interrupt bits
  enum IntBit : uint64_t {
    INT_TX_DONE = 0x00000001,
    INT_TX_ERROR = 0x00000002,
    INT_RX_DONE = 0x00000004,
    INT_RX_ERROR = 0x00000008,
    INT_RX_OVERRUN = 0x00000010,
    INT_MDIO_DONE = 0x00000020,
    INT_LINK_CHANGE = 0x00000040,
  };

  // TX Descriptor (from NetBSD/Linux)
  struct TxDescriptor {
    u64 buffer_addr; // Physical address of buffer
    u32 length;      // Frame length
    u32 flags;       // Control flags
    u32 status;      // Completion status
    u32 reserved;
  };

  // RX Descriptor
  struct RxDescriptor {
    u64 buffer_addr;  // Physical address of buffer
    u32 buffer_size;  // Buffer size
    u32 flags;        // Control flags
    u32 status;       // Completion status
    u32 frame_length; // Received frame length
  };

  // Device interface
  u32 read32(u32 offset) override;
  u16 read16(u32 offset) override;
  u8 read8(u32 offset) override;

  void write32(u32 offset, u32 value) override;
  void write16(u32 offset, u16 value) override;
  void write8(u32 offset, u8 value) override;

  void reset() override;
  void tick(u64 cycles) override;

  // MAC address
  void set_mac_address(const u8 addr[6]);
  void get_mac_address(u8 addr[6]) const;

  // Packet transmission/reception (for integration with network backend)
  bool transmit_packet(const u8 *data, u32 length);
  bool receive_packet(u8 *buffer, u32 buffer_size, u32 &received_length);

  // PHY/MDIO
  u16 mdio_read(u8 phy_addr, u8 reg_addr);
  void mdio_write(u8 phy_addr, u8 reg_addr, u16 value);

private:
  std::array<u64, 0x1000 / 8> regs_ = {}; // 4KB register space (64-bit regs)

  // MAC address
  u8 mac_addr_[6] = {0x08, 0x00, 0x69, 0x00, 0x00, 0x00}; // SGI OUI

  // Descriptor rings
  std::vector<TxDescriptor> tx_descriptors_;
  std::vector<RxDescriptor> rx_descriptors_;
  size_t tx_head_ = 0, tx_tail_ = 0;
  size_t rx_head_ = 0, rx_tail_ = 0;

  // Packet queues (for network backend integration)
  std::vector<std::vector<u8>> tx_queue_;
  std::vector<std::vector<u8>> rx_queue_;

  // Link status
  bool link_up_ = false;
  bool full_duplex_ = true;
  bool speed_100mbps_ = true;
};

} // namespace o2emu::devices