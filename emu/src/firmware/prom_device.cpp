/**
 * @file prom_device.cpp
 * @brief Bus-facing System ROM (PROM) device implementation
 */

#include <algorithm>
#include <o2emu/firmware/prom_device.h>
#include <o2emu/logging/logger.h>

namespace o2emu::firmware {

PROMDevice::PROMDevice(const u8 *data, size_t size)
    : Device("PROM", ip32::PHYS_SYSTEM_ROM, ip32::SYSTEM_ROM_WINDOW_SIZE) {
  update_data(data, size);
}

PROMDevice::PROMDevice(const std::vector<u8> &data)
    : Device("PROM", ip32::PHYS_SYSTEM_ROM, ip32::SYSTEM_ROM_WINDOW_SIZE),
      rom_data_(data) {}

void PROMDevice::update_data(const u8 *data, size_t size) {
  if (data && size > 0) {
    rom_data_.assign(data, data + size);
  } else {
    rom_data_.clear();
  }
}

u32 PROMDevice::read32(u32 offset) {
  if (offset + 4 <= rom_data_.size()) {
    return (static_cast<u32>(rom_data_[offset]) << 24) |
           (static_cast<u32>(rom_data_[offset + 1]) << 16) |
           (static_cast<u32>(rom_data_[offset + 2]) << 8) |
           static_cast<u32>(rom_data_[offset + 3]);
  }

  u32 val = 0;
  for (u32 i = 0; i < 4; ++i) {
    u32 b = (offset + i < rom_data_.size()) ? rom_data_[offset + i] : 0;
    val = (val << 8) | b;
  }
  return val;
}

u16 PROMDevice::read16(u32 offset) {
  if (offset + 2 <= rom_data_.size()) {
    return (static_cast<u16>(rom_data_[offset]) << 8) |
           static_cast<u16>(rom_data_[offset + 1]);
  }

  u16 val = 0;
  for (u32 i = 0; i < 2; ++i) {
    u8 b = (offset + i < rom_data_.size()) ? rom_data_[offset + i] : 0;
    val = (val << 8) | b;
  }
  return val;
}

u8 PROMDevice::read8(u32 offset) {
  if (offset < rom_data_.size()) {
    return rom_data_[offset];
  }
  return 0;
}

void PROMDevice::write32(u32 offset, u32 value) {
  O2EMU_LOG_DEBUG_F("Attempted write32 to read-only PROM at offset 0x%08X (val=0x%08X)",
                    offset, value);
}

void PROMDevice::write16(u32 offset, u16 value) {
  O2EMU_LOG_DEBUG_F("Attempted write16 to read-only PROM at offset 0x%08X (val=0x%04X)",
                    offset, value);
}

void PROMDevice::write8(u32 offset, u8 value) {
  O2EMU_LOG_DEBUG_F("Attempted write8 to read-only PROM at offset 0x%08X (val=0x%02X)",
                    offset, value);
}

void PROMDevice::reset() {
  // ROM has no volatile register state
}

} // namespace o2emu::firmware
