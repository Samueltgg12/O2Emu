#pragma once

/**
 * @file crime_device.h
 * @brief Bus-facing CRIME memory and system controller device adapter
 */

#include <o2emu/devices/device.h>
#include <o2emu/memory/crime.h>

namespace o2emu::memory {

class CRIMEDevice final : public devices::Device {
public:
  explicit CRIMEDevice(CRIME &crime);
  ~CRIMEDevice() override = default;

  u32 read32(u32 offset) override;
  u16 read16(u32 offset) override;
  u8 read8(u32 offset) override;

  void write32(u32 offset, u32 value) override;
  void write16(u32 offset, u16 value) override;
  void write8(u32 offset, u8 value) override;

  void reset() override;

private:
  CRIME &crime_;
};

} // namespace o2emu::memory
