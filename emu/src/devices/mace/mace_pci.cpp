/**
 * @file mace_pci.cpp
 * @brief MACE PCI bridge implementation
 */

#include <cstring>
#include <o2emu/devices/mace/mace_pci.h>
#include <o2emu/logging/logger.h>

namespace o2emu::devices {

MACEPCI::MACEPCI(MACE &mace) : mace_(mace) { reset(); }

MACEPCI::~MACEPCI() = default;

void MACEPCI::reset() {
  std::memset(config_space_, 0, sizeof(config_space_));
  std::memset(regs_, 0, sizeof(regs_));

  // PCI config space defaults
  // Vendor ID: SGI (0x10A9)
  config_space_[0x00] = 0x10A9;
  // Device ID: MACE PCI bridge
  config_space_[0x02] = 0x0001;
  // Command register
  config_space_[0x04] = 0x0000;
  // Status register
  config_space_[0x06] = 0x0280;
  // Revision ID
  config_space_[0x08] = 0x01;
  // Class code: PCI bridge
  config_space_[0x09] = 0x06;
  config_space_[0x0A] = 0x00;
  config_space_[0x0B] = 0x04;
  // Cache line size
  config_space_[0x0C] = 0x08;
  // Latency timer
  config_space_[0x0D] = 0x00;
  // Header type: PCI-to-PCI bridge
  config_space_[0x0E] = 0x01;
  // BIST
  config_space_[0x0F] = 0x00;

  // Base address registers (not used for bridge)
  for (int i = 0; i < 2; ++i) {
    config_space_[0x10 + i * 4] = 0;
  }

  // Primary bus number
  config_space_[0x18] = 0x00;
  // Secondary bus number
  config_space_[0x19] = 0x01;
  // Subordinate bus number
  config_space_[0x1A] = 0x01;
  // Secondary latency timer
  config_space_[0x1B] = 0x00;
  // I/O base
  config_space_[0x1C] = 0xF0;
  // I/O limit
  config_space_[0x1D] = 0x00;
  // Secondary status
  config_space_[0x1E] = 0x0280;
  // Memory base
  config_space_[0x20] = 0xFFF0;
  // Memory limit
  config_space_[0x22] = 0x0000;
  // Prefetchable memory base
  config_space_[0x24] = 0xFFF0;
  // Prefetchable memory limit
  config_space_[0x26] = 0x0000;
  // Prefetchable base upper 32 bits
  config_space_[0x28] = 0x00000000;
  // Prefetchable limit upper 32 bits
  config_space_[0x2C] = 0x00000000;
  // I/O base upper 16 bits
  config_space_[0x30] = 0x0000;
  // I/O limit upper 16 bits
  config_space_[0x32] = 0x0000;
  // Capabilities pointer
  config_space_[0x34] = 0x00;
  // Expansion ROM base
  config_space_[0x38] = 0x00000000;
  // Bridge control
  config_space_[0x3C] = 0x0000;
  // Interrupt line
  config_space_[0x3C] = 0x00;
  // Interrupt pin
  config_space_[0x3D] = 0x01;
  // Min grant
  config_space_[0x3E] = 0x00;
  // Max latency
  config_space_[0x3F] = 0x00;
}

u32 MACEPCI::read_reg(u32 offset) {
  if (offset < sizeof(regs_)) {
    return regs_[offset / 4];
  }
  return 0;
}

void MACEPCI::write_reg(u32 offset, u32 value) {
  if (offset >= sizeof(regs_))
    return;

  u32 reg = offset / 4;
  regs_[reg] = value;
}

void MACEPCI::config_write(u32 reg, u32 value) {
  // Handle PCI config space writes from MACE
  switch (reg) {
  case 0x1000: // PCI_CONFIG
    // Handle bridge configuration
    break;
  case 0x1008: // PCI_IO_BASE
    config_space_[0x1C] = (value >> 8) & 0xFF;
    break;
  case 0x1010: // PCI_MEM_BASE
    config_space_[0x20] = (value >> 16) & 0xFFF0;
    break;
  case 0x1018: // PCI_PREFETCH_BASE
    config_space_[0x24] = (value >> 16) & 0xFFF0;
    break;
  }
}

bool MACEPCI::read(u32 offset, u32 size, u32 &value) {
  if (offset < 0x10000) {
    // PCI config space access
    if (offset < 256) {
      value = config_space_[offset];
      return true;
    }
  }
  return false;
}

bool MACEPCI::write(u32 offset, u32 size, u32 value) {
  if (offset < 0x10000) {
    // PCI config space access
    if (offset < 256) {
      config_space_[offset] = value;
      return true;
    }
  }
  return false;
}

void MACEPCI::tick(u64 cycles) {
  // PCI bridge doesn't need periodic updates
}

u32 MACEPCI::interrupt_status() const {
  return 0; // PCI interrupts handled by MACE interrupt controller
}

} // namespace o2emu::devices