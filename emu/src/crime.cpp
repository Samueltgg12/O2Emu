#include "o2emu/bus.h"
#include "o2emu/logging.h"
#include "o2emu/memory_map.h"
#include "o2emu/tracing.h"
#include <cstring>

namespace o2emu {

class CrimeDevice : public BusDevice {
public:
  CrimeDevice() : BusDevice() {
    // CRIME CPU interface at 0x14000000, size 0x100000 (1MB)
    // CRIME RE at 0x15000000, size 0x100000 (1MB)
  }

  uint32_t base_address() const override { return PHYS_CRIME_BASE; }
  uint32_t size() const override { return 0x200000; } // CPU + RE
  std::string name() const override { return "CRIME"; }

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
    std::fill(cpu_regs_.begin(), cpu_regs_.end(), 0);
    std::fill(re_regs_.begin(), re_regs_.end(), 0);
    cpu_regs_[static_cast<size_t>(CrimeCpuReg::ID)] = 0x01; // CRIME revision 1
    cpu_regs_[static_cast<size_t>(CrimeCpuReg::CONTROL)] = CRM_CTRL_ENDIAN_BIG;
  }

private:
  // CPU interface registers (0x0000 - 0x0fff)
  std::array<uint32_t, 0x1000 / 4> cpu_regs_{};

  // RE registers (0x10000 - 0x1ffff) - 64KB per page, 5 pages
  std::array<uint32_t, 0x10000 / 4> re_regs_{};

  uint32_t read_cpu_reg(uint32_t offset);
  void write_cpu_reg(uint32_t offset, uint32_t val);
  uint32_t read_re_reg(uint32_t offset);
  void write_re_reg(uint32_t offset, uint32_t val);
};

uint32_t CrimeDevice::read32(uint32_t offset) {
  if (offset < 0x10000) {
    // CPU interface (0x14000000 - 0x1400ffff)
    return read_cpu_reg(offset);
  } else {
    // RE (0x15000000 - 0x1501ffff)
    return read_re_reg(offset - 0x10000);
  }
}

void CrimeDevice::write32(uint32_t offset, uint32_t val) {
  if (offset < 0x10000) {
    write_cpu_reg(offset, val);
  } else {
    write_re_reg(offset - 0x10000, val);
  }
}

uint32_t CrimeDevice::read_cpu_reg(uint32_t offset) {
  if (offset >= cpu_regs_.size() * 4)
    return 0;
  size_t idx = offset / 4;
  uint32_t val = cpu_regs_[idx];

  // Special handling for certain registers
  switch (static_cast<CrimeCpuReg>(offset)) {
  case CrimeCpuReg::TIME: {
    // Crime timer runs at 66.6 MHz - simulate with cycle counter
    static uint64_t timer_base = 0;
    val = static_cast<uint32_t>(timer_base++);
    break;
  }
  case CrimeCpuReg::INTSTAT: {
    // Return pending interrupts
    val = cpu_regs_[static_cast<size_t>(CrimeCpuReg::INTSTAT)] &
          cpu_regs_[static_cast<size_t>(CrimeCpuReg::INTMASK)];
    break;
  }
  default:
    break;
  }

  O2E_LOG_READ(CRIME, "CPU_REG[0x{:04x}]", val);
  Tracer::instance().trace_register_access(
      {0, "CRIME_CPU", offset, 4, false, val, 0});
  return val;
}

void CrimeDevice::write_cpu_reg(uint32_t offset, uint32_t val) {
  if (offset >= cpu_regs_.size() * 4)
    return;
  size_t idx = offset / 4;

  O2E_LOG_WRITE(CRIME, "CPU_REG[0x{:04x}]", val);
  Tracer::instance().trace_register_access(
      {0, "CRIME_CPU", offset, 4, true, val, 0});

  switch (static_cast<CrimeCpuReg>(offset)) {
  case CrimeCpuReg::CONTROL: {
    // Handle reset bits
    if (val & CRM_CTRL_HARD_RESET) {
      O2E_WARN(LogCategory::CRIME, "HARD_RESET requested");
      reset();
      return;
    }
    if (val & CRM_CTRL_SOFT_RESET) {
      O2E_WARN(LogCategory::CRIME, "SOFT_RESET requested");
      // Soft reset - clear some state but not all
    }
    cpu_regs_[idx] = val;
    break;
  }
  case CrimeCpuReg::INTMASK:
    cpu_regs_[idx] = val;
    break;
  case CrimeCpuReg::SOFTINT:
    cpu_regs_[static_cast<size_t>(CrimeCpuReg::INTSTAT)] |= val;
    break;
  case CrimeCpuReg::HARDINT:
    cpu_regs_[static_cast<size_t>(CrimeCpuReg::INTSTAT)] |= val;
    break;
  case CrimeCpuReg::DOG:
    cpu_regs_[idx] = val;
    break;
  case CrimeCpuReg::CPU_ERROR_ENA:
    cpu_regs_[idx] = val & 0x7;
    break;
  case CrimeCpuReg::MEM_CONTROL:
    cpu_regs_[idx] = val & 0x3;
    break;
  case CrimeCpuReg::MEM_BANK_CTRL_0:
  case CrimeCpuReg::MEM_BANK_CTRL_1:
  case CrimeCpuReg::MEM_BANK_CTRL_2:
  case CrimeCpuReg::MEM_BANK_CTRL_3:
  case CrimeCpuReg::MEM_BANK_CTRL_4:
  case CrimeCpuReg::MEM_BANK_CTRL_5:
  case CrimeCpuReg::MEM_BANK_CTRL_6:
  case CrimeCpuReg::MEM_BANK_CTRL_7:
    cpu_regs_[idx] = val & 0x11f;
    break;
  case CrimeCpuReg::MEM_REFRESH_CNTR:
    cpu_regs_[idx] = val & 0x7ff;
    break;
  default:
    cpu_regs_[idx] = val;
    break;
  }
}

uint32_t CrimeDevice::read_re_reg(uint32_t offset) {
  if (offset >= re_regs_.size() * 4)
    return 0;
  size_t idx = offset / 4;
  uint32_t val = re_regs_[idx];

  O2E_LOG_READ(RE, "RE_REG[0x{:04x}]", val);
  Tracer::instance().trace_register_access(
      {0, "CRIME_RE", offset, 4, false, val, 0});
  return val;
}

void CrimeDevice::write_re_reg(uint32_t offset, uint32_t val) {
  if (offset >= re_regs_.size() * 4)
    return;
  size_t idx = offset / 4;

  O2E_LOG_WRITE(RE, "RE_REG[0x{:04x}]", val);
  Tracer::instance().trace_register_access(
      {0, "CRIME_RE", offset, 4, true, val, 0});

  // Handle special RE registers
  switch (offset) {
  case static_cast<uint32_t>(ReIntfBufReg::RESET):
    // Reset interface buffer
    std::fill(re_regs_.begin(), re_regs_.begin() + 0x400 / 4, 0);
    break;
  case static_cast<uint32_t>(RePixPipeReg::PIX_PIPE_FLUSH):
    // Flush pixel pipeline
    break;
  case static_cast<uint32_t>(ReMteReg::FLUSH):
    // Flush MTE
    break;
  default:
    re_regs_[idx] = val;
    break;
  }
}

// Factory function
std::unique_ptr<BusDevice> create_crime_device() {
  auto dev = std::make_unique<CrimeDevice>();
  dev->reset();
  return dev;
}

} // namespace o2emu