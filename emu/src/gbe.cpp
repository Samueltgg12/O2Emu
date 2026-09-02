#include "o2emu/bus.h"
#include "o2emu/logging.h"
#include "o2emu/memory_map.h"
#include "o2emu/tracing.h"
#include <cstring>

namespace o2emu {

class GbeDevice : public BusDevice {
public:
  GbeDevice() = default;

  uint32_t base_address() const override { return PHYS_GBE_BASE; }
  uint32_t size() const override { return 0x100000; } // 1MB register block
  std::string name() const override { return "GBE"; }

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
    // Set chip ID
    regs_[static_cast<size_t>(GbeReg::ID) / 4] = 0x00010000; // GBE rev 1
    // Default dot clock
    regs_[static_cast<size_t>(GbeReg::DOTCLOCK) / 4] = 0x00100000; // RUN bit
  }

private:
  std::array<uint32_t, 0x100000 / 4> regs_{};

  uint32_t read_reg(uint32_t offset);
  void write_reg(uint32_t offset, uint32_t val);
};

uint32_t GbeDevice::read32(uint32_t offset) { return read_reg(offset); }

void GbeDevice::write32(uint32_t offset, uint32_t val) {
  write_reg(offset, val);
}

uint32_t GbeDevice::read_reg(uint32_t offset) {
  if (offset >= regs_.size() * 4)
    return 0;
  size_t idx = offset / 4;
  uint32_t val = regs_[idx];

  // Special handling for certain registers
  switch (static_cast<GbeReg>(offset)) {
  case GbeReg::VT_XY: {
    // Simulate current dot coordinates
    static uint32_t xy = 0;
    val = xy++;
    break;
  }
  case GbeReg::CM_FIFO: {
    // Cmap FIFO status - return not full
    val = 0;
    break;
  }
  default:
    break;
  }

  O2E_LOG_READ(GBE, "REG[0x{:05x}]", val);
  Tracer::instance().trace_register_access(
      {0, "GBE", offset, 4, false, val, 0});
  return val;
}

void GbeDevice::write_reg(uint32_t offset, uint32_t val) {
  if (offset >= regs_.size() * 4)
    return;
  size_t idx = offset / 4;

  O2E_LOG_WRITE(GBE, "REG[0x{:05x}]", val);
  Tracer::instance().trace_register_access({0, "GBE", offset, 4, true, val, 0});

  switch (static_cast<GbeReg>(offset)) {
  case GbeReg::CTRLSTAT: {
    // Control/status register
    regs_[idx] = val;
    break;
  }
  case GbeReg::DOTCLOCK: {
    // Dot clock PLL - validate M, N, P values
    regs_[idx] = val;
    break;
  }
  case GbeReg::VT_XYMAX: {
    // Video timing max coordinates
    regs_[idx] = val;
    break;
  }
  case GbeReg::VT_VSYNC:
  case GbeReg::VT_HSYNC:
  case GbeReg::VT_VBLANK:
  case GbeReg::VT_HBLANK:
  case GbeReg::VT_FLAGS:
  case GbeReg::VT_F2RF_LOCK:
  case GbeReg::VT_INTR01:
  case GbeReg::VT_INTR23:
  case GbeReg::FP_HDRV:
  case GbeReg::FP_VDRV:
  case GbeReg::FP_DE:
  case GbeReg::VT_HPIXEN:
  case GbeReg::VT_VPIXEN:
  case GbeReg::VT_HCMAP:
  case GbeReg::VT_VCMAP:
  case GbeReg::DID_START_XY:
  case GbeReg::CRS_START_XY:
  case GbeReg::VC_START_XY:
  case GbeReg::OVR_WIDTH_TILE:
  case GbeReg::OVR_CONTROL:
  case GbeReg::FRM_SIZE_TILE:
  case GbeReg::FRM_SIZE_PIXEL:
  case GbeReg::FRM_CONTROL:
  case GbeReg::DID_CONTROL:
  case GbeReg::VC_LR:
  case GbeReg::VC_TB:
  case GbeReg::VC_FILTERS:
  case GbeReg::VC_CONTROL:
  case GbeReg::CRS_POS:
  case GbeReg::CRS_CTL:
    regs_[idx] = val;
    break;
  default:
    regs_[idx] = val;
    break;
  }
}

// Factory function
std::unique_ptr<BusDevice> create_gbe_device() {
  auto dev = std::make_unique<GbeDevice>();
  dev->reset();
  return dev;
}

} // namespace o2emu