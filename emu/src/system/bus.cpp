/**
 * @file bus.cpp
 * @brief System bus implementation
 */

#include <algorithm>
#include <o2emu/logging/logger.h>
#include <o2emu/system/bus.h>

namespace o2emu::system {

Bus::Bus() = default;

void Bus::attach_device(std::unique_ptr<devices::Device> device) {
  if (!device) {
    return;
  }

  u32 base = device->base_addr();
  u32 size = device->size();

  // Store the device pointer before moving ownership
  devices::Device *device_ptr = device.get();

  // Add to owned devices (takes ownership)
  owned_devices_.push_back(std::move(device));

  // Add to device lookup table
  devices_.push_back({base, size, device_ptr});

  // Sort by base address for efficient lookup
  std::sort(devices_.begin(), devices_.end(),
            [](const DeviceEntry &a, const DeviceEntry &b) {
              return a.base < b.base;
            });

  O2EMU_LOG_DEBUG_F("Attached device: %s at 0x%08X size 0x%08X",
                    device_ptr->name().c_str(), base, size);
}

devices::Device *Bus::find_device(u32 phys_addr) const {
  for (const auto &entry : devices_) {
    if (phys_addr >= entry.base && phys_addr < entry.base + entry.size) {
      return entry.device;
    }
  }
  return nullptr;
}

u32 Bus::read32(u32 phys_addr) {
  devices::Device *device = find_device(phys_addr);
  if (device) {
    u32 offset = phys_addr - device->base_addr();
    return device->read32(offset);
  }

  O2EMU_LOG_WARN_F("Bus read32 from unmapped address: 0x%08X", phys_addr);
  return 0xFFFFFFFF;
}

u16 Bus::read16(u32 phys_addr) {
  devices::Device *device = find_device(phys_addr);
  if (device) {
    u32 offset = phys_addr - device->base_addr();
    return device->read16(offset);
  }

  O2EMU_LOG_WARN_F("Bus read16 from unmapped address: 0x%08X", phys_addr);
  return 0xFFFF;
}

u8 Bus::read8(u32 phys_addr) {
  devices::Device *device = find_device(phys_addr);
  if (device) {
    u32 offset = phys_addr - device->base_addr();
    return device->read8(offset);
  }

  O2EMU_LOG_WARN_F("Bus read8 from unmapped address: 0x%08X", phys_addr);
  return 0xFF;
}

void Bus::write32(u32 phys_addr, u32 value) {
  devices::Device *device = find_device(phys_addr);
  if (device) {
    u32 offset = phys_addr - device->base_addr();
    device->write32(offset, value);
    return;
  }

  O2EMU_LOG_WARN_F("Bus write32 to unmapped address: 0x%08X value=0x%08X",
                   phys_addr, value);
}

void Bus::write16(u32 phys_addr, u16 value) {
  devices::Device *device = find_device(phys_addr);
  if (device) {
    u32 offset = phys_addr - device->base_addr();
    device->write16(offset, value);
    return;
  }

  O2EMU_LOG_WARN_F("Bus write16 to unmapped address: 0x%08X value=0x%04X",
                   phys_addr, value);
}

void Bus::write8(u32 phys_addr, u8 value) {
  devices::Device *device = find_device(phys_addr);
  if (device) {
    u32 offset = phys_addr - device->base_addr();
    device->write8(offset, value);
    return;
  }

  O2EMU_LOG_WARN_F("Bus write8 to unmapped address: 0x%08X value=0x%02X",
                   phys_addr, value);
}

void Bus::tick(u64 cycles) {
  for (const auto &entry : devices_) {
    entry.device->tick(cycles);
  }
}

void Bus::reset() {
  for (const auto &entry : devices_) {
    entry.device->reset();
  }
}

} // namespace o2emu::system