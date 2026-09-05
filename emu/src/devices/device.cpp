/**
 * @file device.cpp
 * @brief Base device class implementation
 */

#include <o2emu/devices/device.h>
#include <o2emu/logging/logger.h>

namespace o2emu::devices {

Device::Device(const std::string &name, u32 base_addr, u32 size)
    : name_(name), base_addr_(base_addr), size_(size), enabled_(true) {}

void Device::reset() { enabled_ = true; }

} // namespace o2emu::devices