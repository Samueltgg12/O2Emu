#pragma once

/**
 * @file prom_device.h
 * @brief Bus-facing System ROM (PROM) device for SGI O2 (IP32)
 */

#include <o2emu/devices/device.h>
#include <o2emu/o2emu.h>
#include <vector>

namespace o2emu::firmware {

class PROMDevice final : public devices::Device {
public:
  PROMDevice(const u8 *data, size_t size);
  explicit PROMDevice(const std::vector<u8> &data);
  ~PROMDevice() override = default;

  u32 read32(u32 offset) override;
  u16 read16(u32 offset) override;
  u8 read8(u32 offset) override;

  void write32(u32 offset, u32 value) override;
  void write16(u32 offset, u16 value) override;
  void write8(u32 offset, u8 value) override;

  void reset() override;

  void update_data(const u8 *data, size_t size);

private:
  std::vector<u8> rom_data_;
};

} // namespace o2emu::firmware
