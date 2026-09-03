#include "o2emu/bus.h"
#include "o2emu/logging.h"
#include "o2emu/memory_map.h"
#include "o2emu/tracing.h"
#include <cstring>

namespace o2emu {

class MaceDevice : public BusDevice {
public:
  MaceDevice() = default;

  uint32_t base_address() const override { return PHYS_MACE_BASE; }
  uint32_t size() const override { return 0x1000000; } // 16MB register block
  std::string name() const override { return "MACE"; }

  uint8_t read8(uint32_t offset) override { return read32(offset) & 0xff; }
  uint16_t read16(uint32_t offset) override { return read32(offset) & 0xffff; }
  uint32_t read32(uint32_t offset) override;
  uint64_t read64(uint32_t offset) override {
    uint64_t lo = read32(offset);
    uint64_t hi = read32(offset + 4);
    return (hi << 32) | lo;
  }

  void write8(uint32_t offset, uint8_t val) override { write32(offset, val); }
  void write16(uint32_t offset, uint16_t val) override { write32(offset, val); }
  void write32(uint32_t offset, uint32_t val) override;
  void write64(uint32_t offset, uint64_t val) override {
    write32(offset, static_cast<uint32_t>(val));
    write32(offset + 4, static_cast<uint32_t>(val >> 32));
  }

  void reset() override {
    std::fill(regs_.begin(), regs_.end(), 0);
    // PCI revision info
    regs_[static_cast<size_t>(MaceReg::PCI_REV_INFO_R) / 4] =
        0x00010001; // rev 1
  }

private:
  std::array<uint32_t, 0x1000000 / 4> regs_{};

  uint32_t read_reg(uint32_t offset);
  void write_reg(uint32_t offset, uint32_t val);
};

uint32_t MaceDevice::read32(uint32_t offset) { return read_reg(offset); }

void MaceDevice::write32(uint32_t offset, uint32_t val) {
  write_reg(offset, val);
}

uint32_t MaceDevice::read_reg(uint32_t offset) {
  if (offset >= regs_.size() * 4)
    return 0;
  size_t idx = offset / 4;
  uint32_t val = regs_[idx];

  // Special handling for certain registers
  switch (offset) {
  case static_cast<uint32_t>(MaceReg::PCI_CONFIG_DATA): {
    // PCI config space read - return 0 for unimplemented
    val = 0;
    break;
  }
  case static_cast<uint32_t>(MaceReg::PCI_ERROR_FLAGS): {
    // Return error flags
    val = regs_[idx];
    break;
  }
  default:
    break;
  }

  O2E_LOG_READ(MACE, "REG[0x{:06x}]", val);
  Tracer::instance().trace_register_access(
      {0, "MACE", offset, 4, false, val, 0});
  return val;
}

void MaceDevice::write_reg(uint32_t offset, uint32_t val) {
  if (offset >= regs_.size() * 4)
    return;
  size_t idx = offset / 4;

  O2E_LOG_WRITE(MACE, "REG[0x{:06x}]", val);
  Tracer::instance().trace_register_access(
      {0, "MACE", offset, 4, true, val, 0});

  switch (offset) {
  case static_cast<uint32_t>(MaceReg::PCI_CONTROL): {
    regs_[idx] = val & 0x1f;
    break;
  }
  case static_cast<uint32_t>(MaceReg::PCI_FLUSH_W): {
    // PCI flush - no-op in emulation
    break;
  }
  case static_cast<uint32_t>(MaceReg::PCI_CONFIG_ADDR): {
    regs_[idx] = val;
    break;
  }
  case static_cast<uint32_t>(MaceReg::PCI_CONFIG_DATA): {
    // PCI config space write - no-op for unimplemented
    break;
  }
  case static_cast<uint32_t>(MaceReg::ISA_BASE) +
      static_cast<uint32_t>(MaceIsaReg::FLASH_NIC_REG): {
    regs_[idx] = val & 0x7f;
    break;
  }
  case static_cast<uint32_t>(MaceReg::ISA_BASE) +
      static_cast<uint32_t>(MaceIsaReg::INT_MSK_REG): {
    regs_[idx] = val & 0xffff;
    break;
  }
  default:
    regs_[idx] = val;
    break;
  }
}

// Factory function
std::unique_ptr<BusDevice> create_mace_device() {
  auto dev = std::make_unique<MaceDevice>();
  dev->reset();
  return dev;
}

} // namespace o2emu