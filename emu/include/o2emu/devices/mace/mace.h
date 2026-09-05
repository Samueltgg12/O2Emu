#pragma once

/**
 * @file mace.h
 * @brief MACE (Multimedia, Audio and Communications Engine) ASIC
 *
 * Base address: 0x1F000000 (PHYS_BASE_MACE)
 * Based on Linux arch/mips/include/asm/ip32/mace.h and IRIX sources
 */

#include <array>
#include <memory>
#include <o2emu/devices/device.h>
#include <o2emu/memory/memory.h>
#include <o2emu/o2emu.h>

#include "mace_audio.h"
#include "mace_ethernet.h"
#include "mace_isa.h"
#include "mace_pci.h"

namespace o2emu::devices {

class MACE : public Device {
public:
  // MACE register offsets (from Linux arch/mips/include/asm/ip32/mace.h)
  enum Register : u32 {
    REG_ID = 0x0000,
    REG_CONFIG = 0x0004,
    REG_STATUS = 0x0008,
    REG_CONTROL = 0x000C,
    REG_PCI_CONFIG = 0x0010,
    REG_PCI_IO_BASE = 0x0014,
    REG_PCI_MEM_BASE = 0x0018,
    REG_PCI_PREFETCH_BASE = 0x001C,
    REG_ISA_CONFIG = 0x0020,
    REG_ISA_IO_BASE = 0x0024,
    REG_ISA_MEM_BASE = 0x0028,
    REG_INT_STATUS = 0x0030,
    REG_INT_MASK = 0x0034,
    REG_INT_CLEAR = 0x0038,
    REG_INT_ROUTE = 0x003C,
  };

  explicit MACE(Memory &memory);
  ~MACE() override = default;

  // Sub-devices
  MACEPCI &pci() { return *pci_; }
  MACEEthernet &ethernet() { return *ethernet_; }
  MACEAudio &audio() { return *audio_; }
  MACEISA &isa() { return *isa_; }

  // Device interface
  u32 read32(u32 offset) override;
  u16 read16(u32 offset) override;
  u8 read8(u32 offset) override;

  void write32(u32 offset, u32 value) override;
  void write16(u32 offset, u16 value) override;
  void write8(u32 offset, u8 value) override;

  void reset() override;
  void tick(u64 cycles) override;

  // Register access
  u32 read_reg(u32 offset);
  void write_reg(u32 offset, u32 value);

  // Interrupt routing
  void set_interrupt(int line, bool asserted);
  u32 interrupt_status() const { return interrupt_status_; }
  void clear_interrupt(u32 mask) { interrupt_status_ &= ~mask; }

private:
  Memory &memory_;
  std::unique_ptr<MACEPCI> pci_;
  std::unique_ptr<MACEEthernet> ethernet_;
  std::unique_ptr<MACEAudio> audio_;
  std::unique_ptr<MACEISA> isa_;

  // MACE register file
  std::array<u32, 256> regs_{};

  // Interrupt state
  u32 interrupt_status_ = 0;
  u32 interrupt_mask_ = 0;
};

} // namespace o2emu::devices