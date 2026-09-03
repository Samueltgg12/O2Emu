#include "o2emu/bus.h"
#include "o2emu/logging.h"
#include "o2emu/memory_map.h"
#include "o2emu/tracing.h"

namespace o2emu {

class ViceDevice : public BusDevice {
public:
  ViceDevice() = default;

  uint32_t base_address() const override {
    return PHYS_CRIME_BASE + 0x00080000u;
  }
  uint32_t size() const override { return 0x10000; }
  std::string name() const override { return "VICE"; }

  uint8_t read8(uint32_t offset) override { return read32(offset) & 0xff; }
  uint16_t read16(uint32_t offset) override { return read32(offset) & 0xffff; }
  uint32_t read32(uint32_t offset) override {
    if (offset >= regs_.size() * 4)
      return 0;
    return regs_[offset / 4];
  }
  uint64_t read64(uint32_t offset) override {
    uint64_t lo = read32(offset);
    uint64_t hi = read32(offset + 4);
    return (hi << 32) | lo;
  }

  void write8(uint32_t offset, uint8_t val) override { write32(offset, val); }
  void write16(uint32_t offset, uint16_t val) override { write32(offset, val); }
  void write32(uint32_t offset, uint32_t val) override {
    if (offset >= regs_.size() * 4)
      return;
    regs_[offset / 4] = val;
  }
  void write64(uint32_t offset, uint64_t val) override {
    write32(offset, static_cast<uint32_t>(val));
    write32(offset + 4, static_cast<uint32_t>(val >> 32));
  }

  void reset() override { std::fill(regs_.begin(), regs_.end(), 0); }

private:
  std::array<uint32_t, 0x10000 / 4> regs_{};
};

std::unique_ptr<BusDevice> create_vice_device() {
  auto dev = std::make_unique<ViceDevice>();
  dev->reset();
  return dev;
}

} // namespace o2emu
