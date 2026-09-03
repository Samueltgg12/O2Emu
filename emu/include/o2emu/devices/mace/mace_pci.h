#pragma once

/**
 * @file mace_pci.h
 * @brief MACE PCI host bridge
 *
 * Offset from MACE base: 0x080000
 * Based on Linux arch/mips/pci/ops-mace.c, pci-ip32.c, fixup-ip32.c
 */

#include <array>
#include <o2emu/devices/device.h>
#include <o2emu/o2emu.h>

namespace o2emu::devices {

class MACEPCI : public Device {
public:
  MACEPCI();
  ~MACEPCI() override;

  // PCI register offsets (from MACE base + 0x080000)
  enum Register : uint32_t {
    PCI_ERROR_ADDR = 0x000000,
    PCI_ERROR_FLAGS = 0x000004,
    PCI_CONTROL = 0x000008,
    PCI_REVISION = 0x00000C,
    PCI_FLUSH = 0x00000C, // Write only
    PCI_CONFIG_ADDR = 0x000CF8,
    PCI_CONFIG_DATA = 0x000CFC,
  };

  // PCI_ERROR_FLAGS bits
  enum ErrorFlag : uint32_t {
    ERR_MASTER_ABORT = 0x80000000,
    ERR_TARGET_ABORT = 0x40000000,
    ERR_DATA_PARITY = 0x20000000,
    ERR_RETRY = 0x10000000,
    ERR_ILLEGAL_CMD = 0x08000000,
    ERR_SYSTEM = 0x04000000,
    ERR_INTERRUPT_TEST = 0x02000000,
    ERR_PARITY = 0x01000000,
    ERR_OVERRUN = 0x00800000,
    ERR_MASTER_ABORT_ADDR_VALID = 0x00080000,
    ERR_TARGET_ABORT_ADDR_VALID = 0x00040000,
    ERR_DATA_PARITY_ADDR_VALID = 0x00020000,
    ERR_RETRY_ADDR_VALID = 0x00010000,
    ERR_SIG_TABORT = 0x00000010,
    ERR_DEVSEL_MASK = 0x000000C0,
    ERR_FBB = 0x00000002,
    ERR_66MHZ = 0x00000001,
  };

  // PCI_CONTROL bits
  enum ControlBit : uint32_t {
    CTRL_INT_MASK = 0x000000FF,
    CTRL_SERR_ENA = 0x00000100,
    CTRL_ARB_N6 = 0x00000200,
    CTRL_PARITY_ERR = 0x00000400,
    CTRL_MRMRA_ENA = 0x00000800,
    CTRL_ARB_N3 = 0x00001000,
    CTRL_ARB_N4 = 0x00002000,
    CTRL_ARB_N5 = 0x00004000,
    CTRL_PARK_LIU = 0x00008000,
    CTRL_INV_INT_MASK = 0x00FF0000,
    CTRL_OVERRUN_INT = 0x01000000,
    CTRL_PARITY_INT = 0x02000000,
    CTRL_SERR_INT = 0x04000000,
    CTRL_IT_INT = 0x08000000,
    CTRL_RE_INT = 0x10000000,
    CTRL_DPED_INT = 0x20000000,
    CTRL_TAR_INT = 0x40000000,
    CTRL_MAR_INT = 0x80000000,
  };

  // PCI address windows
  static constexpr u32 LOW_MEMORY_BASE = 0x1A000000;
  static constexpr u32 LOW_MEMORY_SIZE = 0x02000000; // 32 MB
  static constexpr u32 LOW_IO_BASE = 0x18000000;
  static constexpr u32 HI_MEMORY_BASE = 0x280000000; // 64-bit
  static constexpr u32 HI_IO_BASE = 0x100000000;     // 64-bit

  // Config space access
  u32 config_read(u32 bus, u32 devfn, u32 reg, u32 size);
  void config_write(u32 bus, u32 devfn, u32 reg, u32 size, u32 value);

  // Device interface
  u32 read32(u32 offset) override;
  u16 read16(u32 offset) override;
  u8 read8(u32 offset) override;

  void write32(u32 offset, u32 value) override;
  void write16(u32 offset, u16 value) override;
  void write8(u32 offset, u8 value) override;

  void reset() override;

  // Interrupt mapping (from fixup-ip32.c)
  // Slot 0: SCSI0 (devfn 1<<3), Slot 1: SCSI1 (devfn 2<<3), Slot 2: Expansion
  static int map_irq(int slot, int pin);

private:
  std::array<u32, 0x1000 / 4> regs_ = {};

  // Config space access state
  u32 config_addr_ = 0;
  union {
    u8 b[4];
    u16 w[2];
    u32 l;
  } config_data_;

  // PCI devices (simplified)
  struct PCIDevice {
    u32 vendor_id = 0xFFFF;
    u32 device_id = 0xFFFF;
    u32 command = 0;
    u32 status = 0;
    u32 revision = 0;
    u32 class_code = 0;
    u32 header_type = 0;
    u32 bars[6] = {};
    u32 irq_line = 0;
    u32 irq_pin = 0;
  };
  std::array<PCIDevice, 6> pci_devices_ = {}; // Up to 6 devices
};

} // namespace o2emu::devices