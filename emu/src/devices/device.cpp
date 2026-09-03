/**
 * @file device.cpp
 * @brief Base device class implementation
 */

#include <o2emu/devices/device.h>
#include <o2emu/logging/logger.h>

namespace o2emu::devices {

Device::Device(const std::string &name, u32 base_addr, u32 size)
    : name_(name), base_addr_(base_addr), size_(size), enabled_(true) {}

Device::~Device() = default;

void Device::reset() { enabled_ = true; }

bool Device::read(u32 offset, u32 size, u32 &value) {
  if (!enabled_)
    return false;
  if (offset + size > size_)
    return false;

  value = read_reg(offset);
  return true;
}

bool Device::write(u32 offset, u32 size, u32 value) {
  if (!enabled_)
    return false;
  if (offset + size > size_)
    return false;

  write_reg(offset, value);
  return true;
}

u32 Device::read_reg(u32 offset) {
  // Default implementation - override in derived classes
  return 0;
}

void Device::write_reg(u32 offset, u32 value) {
  // Default implementation - override in derived classes
}

void Device::tick(u64 cycles) {
  // Default implementation - override in derived classes
}

} // namespace o2emu::devices