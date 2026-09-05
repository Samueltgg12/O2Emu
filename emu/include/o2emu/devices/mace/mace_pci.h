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

class MACE;

class MACEPCI {
public:
  explicit MACEPCI(MACE &mace);
  ~MACEPCI();

  // PCI register offsets (from MACE base + 0x080000)
  // All registers are 32-bit, accessed at 4-byte offsets
  enum Register : uint32_t {
    REG_CTRL = 0x0000,
    REG_STATUS = 0x0008,
    REG_CONFIG_ADDR = 0x0010,
    REG_CONFIG_DATA = 0x0018,
    REG_ERROR_ADDR = 0x0020,
    REG_ERROR_FLAGS = 0x0028,
    REG_FLUSH = 0x0030,
    REG_INT_STATUS = 0x0038,
    REG_INT_MASK = 0x0040,
    REG_INT_CLEAR = 0x0048,
    REG_REVISION = 0x0050,
    REG_ARB_CTRL = 0x0058,
    REG_BAR0 = 0x0100,
    REG_BAR1 = 0x0108,
    REG_BAR2 = 0x0110,
    REG_BAR3 = 0x0118,
    REG_BAR4 = 0x0120,
    REG_BAR5 = 0x0128,
    REG_ROM_BASE = 0x0130,
    REG_ROM_LIMIT = 0x0138,
    REG_IO_BASE = 0x0140,
    REG_IO_LIMIT = 0x0148,
    REG_MEM_BASE = 0x0150,
    REG_MEM_LIMIT = 0x0158,
    REG_PREF_BASE = 0x0160,
    REG_PREF_LIMIT = 0x0168,
    REG_PREF_BASE_UPPER = 0x0170,
    REG_PREF_LIMIT_UPPER = 0x0178,
    REG_IO_BASE_UPPER = 0x0180,
    REG_IO_LIMIT_UPPER = 0x0188,
  };

  // PCI address windows
  static constexpr u32 LOW_MEMORY_BASE = 0x1A000000;
  static constexpr u32 LOW_MEMORY_SIZE = 0x02000000; // 32 MB
  static constexpr u32 LOW_IO_BASE = 0x18000000;
  static constexpr u64 HI_MEMORY_BASE = 0x280000000ULL; // 64-bit
  static constexpr u64 HI_IO_BASE = 0x100000000ULL;     // 64-bit

  // Register access
  u32 read_reg(u32 offset);
  void write_reg(u32 offset, u32 value);

  // Config space access
  void config_write(u32 reg, u32 value);

  // Device interface
  bool read(u32 offset, u32 size, u32 &value);
  bool write(u32 offset, u32 size, u32 value);

  void tick(u64 cycles);
  u32 interrupt_status() const;

  void reset();

  // Interrupt mapping (from fixup-ip32.c)
  // Slot 0: SCSI0 (devfn 1<<3), Slot 1: SCSI1 (devfn 2<<3), Slot 2: Expansion
  static int map_irq(int slot, int pin);

private:
  MACE &mace_;
  std::array<u32, 0x200 / 4> regs_ = {};

  // Config space (256 bytes = 64 dwords)
  std::array<u32, 256 / 4> config_data_ = {};

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