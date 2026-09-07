#pragma once

#include <o2emu/devices/device.h>
#include <o2emu/memory/mre.h>

namespace o2emu::memory {

// Bus-facing adapter for the PROM-visible render engine register block.
class MREDevice final : public devices::Device {
public:
  explicit MREDevice(MRE &mre);
  ~MREDevice() override = default;

  u32 read32(u32 offset) override;
  u16 read16(u32 offset) override;
  u8 read8(u32 offset) override;
  void write32(u32 offset, u32 value) override;
  void write16(u32 offset, u16 value) override;
  void write8(u32 offset, u8 value) override;
  void reset() override;

private:
  MRE &mre_;
};

} // namespace o2emu::memory