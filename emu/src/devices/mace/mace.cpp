/**
 * @file mace.cpp
 * @brief MACE (Multimedia, Audio and Communications Engine) ASIC implementation
 *
 * Base address: 0x1F000000 (PHYS_BASE_MACE)
 * Based on Linux arch/mips/include/asm/ip32/mace.h and IRIX sources
 */

#include <o2emu/devices/mace/mace.h>
#include <o2emu/logging/logger.h>

namespace o2emu::devices {

MACE::MACE(o2emu::memory::Memory &memory)
    : Device("MACE", 0x1F000000, 0x1000000), memory_(memory) {
  // Initialize sub-devices
  pci_ = std::make_unique<MACEPCI>(*this);
  ethernet_ = std::make_unique<MACEEthernet>(*this);
  audio_ = std::make_unique<MACEAudio>(*this);
  isa_ = std::make_unique<MACEISA>(*this);

  // Initialize register file
  regs_.fill(0);
}

u32 MACE::read32(u32 offset) {
  // Route to sub-devices based on offset
  if (offset < 0x10000) {
    // MACE core registers
    if (offset >= regs_.size() * 4) {
      return 0;
    }
    return regs_[offset / 4];
  } else if (offset < 0x20000) {
    // PCI
    return pci_->read_reg(offset - 0x10000);
  } else if (offset < 0x30000) {
    // ISA
    return isa_->read_reg(offset - 0x20000);
  } else if (offset < 0x40000) {
    // Ethernet
    return ethernet_->read_reg(offset - 0x30000);
  } else if (offset < 0x50000) {
    // Audio
    return audio_->read_reg(offset - 0x40000);
  }
  return 0;
}

u16 MACE::read16(u32 offset) {
  return static_cast<u16>(read32(offset) >> ((offset & 2) * 8));
}

u8 MACE::read8(u32 offset) {
  return static_cast<u8>(read32(offset) >> ((offset & 3) * 8));
}

void MACE::write32(u32 offset, u32 value) {
  if (offset < 0x10000) {
    // MACE core registers
    if (offset >= regs_.size() * 4) {
      return;
    }
    u32 reg_idx = offset / 4;
    regs_[reg_idx] = value;

    // Handle special registers
    switch (reg_idx) {
    case REG_CONTROL:
      // Control register write
      break;
    case REG_INT_CLEAR:
      interrupt_status_ &= ~value;
      regs_[REG_INT_STATUS] = interrupt_status_;
      break;
    case REG_INT_MASK:
      interrupt_mask_ = value;
      break;
    }
    return;
  } else if (offset < 0x20000) {
    pci_->write_reg(offset - 0x10000, value);
    return;
  } else if (offset < 0x30000) {
    isa_->write_reg(offset - 0x20000, value);
    return;
  } else if (offset < 0x40000) {
    ethernet_->write_reg(offset - 0x30000, value);
    return;
  } else if (offset < 0x50000) {
    audio_->write_reg(offset - 0x40000, value);
    return;
  }
}

void MACE::write16(u32 offset, u16 value) {
  u32 current = read32(offset & ~3);
  u32 shift = (offset & 2) * 8;
  current = (current & ~(0xFFFF << shift)) | (static_cast<u32>(value) << shift);
  write32(offset & ~3, current);
}

void MACE::write8(u32 offset, u8 value) {
  u32 current = read32(offset & ~3);
  u32 shift = (offset & 3) * 8;
  current = (current & ~(0xFF << shift)) | (static_cast<u32>(value) << shift);
  write32(offset & ~3, current);
}

u32 MACE::read_reg(u32 offset) { return read32(offset); }

void MACE::write_reg(u32 offset, u32 value) { write32(offset, value); }

void MACE::reset() {
  regs_.fill(0);
  interrupt_status_ = 0;
  interrupt_mask_ = 0;

  pci_->reset();
  ethernet_->reset();
  audio_->reset();
  isa_->reset();
}

void MACE::tick(u64 cycles) {
  pci_->tick(cycles);
  ethernet_->tick(cycles);
  audio_->tick(cycles);
  isa_->tick(cycles);
}

void MACE::set_interrupt(int line, bool asserted) {
  if (asserted) {
    interrupt_status_ |= (1u << line);
  } else {
    interrupt_status_ &= ~(1u << line);
  }
  regs_[REG_INT_STATUS] = interrupt_status_;
}

} // namespace o2emu::devices