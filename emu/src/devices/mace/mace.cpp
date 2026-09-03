/**
 * @file mace.cpp
 * @brief MACE ASIC implementation (PCI bridge, ISA, PS/2, Ethernet, Audio)
 */

#include <cstring>
#include <o2emu/devices/mace/mace.h>
#include <o2emu/logging/logger.h>

namespace o2emu::devices {

MACE::MACE()
    : Device("MACE", 0x70000000, 0x10000000) // 256MB at 0x70000000
      ,
      pci_(std::make_unique<MACEPCI>(*this)),
      ethernet_(std::make_unique<MACEEthernet>(*this)),
      audio_(std::make_unique<MACEAudio>(*this)),
      isa_(std::make_unique<MACEISA>(*this)) {
  reset();
}

MACE::~MACE() = default;

void MACE::reset() {
  Device::reset();
  std::memset(regs_, 0, sizeof(regs_));

  // MACE register defaults
  // MACE_ID: 0x0000
  regs_[REG_ID] = 0x00000001; // MACE revision 1

  // MACE_CONFIG: 0x0008
  regs_[REG_CONFIG] = 0x00000000;

  // MACE_STATUS: 0x0010
  regs_[REG_STATUS] = 0x00000000;

  // MACE_CONTROL: 0x0018
  regs_[REG_CONTROL] = 0x00000000;

  // PCI configuration
  // MACE_PCI_CONFIG: 0x1000
  regs_[REG_PCI_CONFIG] = 0x00000000;

  // MACE_PCI_IO_BASE: 0x1008
  regs_[REG_PCI_IO_BASE] = 0x00000000;

  // MACE_PCI_MEM_BASE: 0x1010
  regs_[REG_PCI_MEM_BASE] = 0x00000000;

  // MACE_PCI_PREFETCH_BASE: 0x1018
  regs_[REG_PCI_PREFETCH_BASE] = 0x00000000;

  // ISA configuration
  // MACE_ISA_CONFIG: 0x2000
  regs_[REG_ISA_CONFIG] = 0x00000000;

  // MACE_ISA_IO_BASE: 0x2008
  regs_[REG_ISA_IO_BASE] = 0x00000000;

  // MACE_ISA_MEM_BASE: 0x2010
  regs_[REG_ISA_MEM_BASE] = 0x00000000;

  // Interrupt controller
  // MACE_INT_STATUS: 0x3000
  regs_[REG_INT_STATUS] = 0x00000000;

  // MACE_INT_MASK: 0x3008
  regs_[REG_INT_MASK] = 0x00000000;

  // MACE_INT_CLEAR: 0x3010
  regs_[REG_INT_CLEAR] = 0x00000000;

  // MACE_INT_ROUTE: 0x3018
  regs_[REG_INT_ROUTE] = 0x00000000;

  // Reset sub-devices
  pci_->reset();
  ethernet_->reset();
  audio_->reset();
  isa_->reset();
}

u32 MACE::read_reg(u32 offset) {
  if (offset < sizeof(regs_)) {
    return regs_[offset / 4];
  }
  return 0;
}

void MACE::write_reg(u32 offset, u32 value) {
  if (offset >= sizeof(regs_))
    return;

  u32 reg = offset / 4;

  switch (reg) {
  case REG_CONTROL:
    regs_[reg] = value;
    break;

  case REG_PCI_CONFIG:
  case REG_PCI_IO_BASE:
  case REG_PCI_MEM_BASE:
  case REG_PCI_PREFETCH_BASE:
    regs_[reg] = value;
    pci_->config_write(reg, value);
    break;

  case REG_ISA_CONFIG:
  case REG_ISA_IO_BASE:
  case REG_ISA_MEM_BASE:
    regs_[reg] = value;
    isa_->config_write(reg, value);
    break;

  case REG_INT_MASK:
    regs_[reg] = value;
    break;

  case REG_INT_CLEAR:
    regs_[REG_INT_STATUS] &= ~value;
    break;

  case REG_INT_ROUTE:
    regs_[reg] = value;
    break;

  default:
    regs_[reg] = value;
    break;
  }
}

bool MACE::read(u32 offset, u32 size, u32 &value) {
  // Check sub-device ranges
  if (offset >= 0x10000 && offset < 0x20000) {
    return pci_->read(offset - 0x10000, size, value);
  }
  if (offset >= 0x20000 && offset < 0x30000) {
    return isa_->read(offset - 0x20000, size, value);
  }
  if (offset >= 0x40000 && offset < 0x50000) {
    return ethernet_->read(offset - 0x40000, size, value);
  }
  if (offset >= 0x50000 && offset < 0x60000) {
    return audio_->read(offset - 0x50000, size, value);
  }

  return Device::read(offset, size, value);
}

bool MACE::write(u32 offset, u32 size, u32 value) {
  // Check sub-device ranges
  if (offset >= 0x10000 && offset < 0x20000) {
    return pci_->write(offset - 0x10000, size, value);
  }
  if (offset >= 0x20000 && offset < 0x30000) {
    return isa_->write(offset - 0x20000, size, value);
  }
  if (offset >= 0x40000 && offset < 0x50000) {
    return ethernet_->write(offset - 0x40000, size, value);
  }
  if (offset >= 0x50000 && offset < 0x60000) {
    return audio_->write(offset - 0x50000, size, value);
  }

  return Device::write(offset, size, value);
}

void MACE::tick(u64 cycles) {
  Device::tick(cycles);
  pci_->tick(cycles);
  ethernet_->tick(cycles);
  audio_->tick(cycles);
  isa_->tick(cycles);

  // Update interrupt status from sub-devices
  u32 pci_int = pci_->interrupt_status();
  u32 eth_int = ethernet_->interrupt_status();
  u32 aud_int = audio_->interrupt_status();
  u32 isa_int = isa_->interrupt_status();

  regs_[REG_INT_STATUS] =
      (pci_int | (eth_int << 8) | (aud_int << 16) | (isa_int << 24));
}

u32 MACE::interrupt_status() const {
  return regs_[REG_INT_STATUS] & regs_[REG_INT_MASK];
}

void MACE::clear_interrupt(u32 bit) { regs_[REG_INT_STATUS] &= ~(1 << bit); }

} // namespace o2emu::devices