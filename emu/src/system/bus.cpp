/**
 * @file bus.cpp
 * @brief System bus implementation
 */

#include <algorithm>
#include <o2emu/logging/logger.h>
#include <o2emu/system/bus.h>

namespace o2emu::system {

Bus::Bus() : devices_(), memory_(nullptr) {}

Bus::~Bus() = default;

void Bus::attach_memory(memory::Memory *memory) { memory_ = memory; }

void Bus::attach_device(Device *device) {
  if (device) {
    devices_.push_back(device);
    // Sort by base address for efficient lookup
    std::sort(devices_.begin(), devices_.end(), [](Device *a, Device *b) {
      return a->base_addr() < b->base_addr();
    });
    O2EMU_LOG_DEBUG("Attached device: " << device->name() << " at 0x"
                                        << std::hex << device->base_addr()
                                        << " size 0x" << device->size()
                                        << std::dec);
  }
}

void Bus::detach_device(Device *device) {
  auto it = std::find(devices_.begin(), devices_.end(), device);
  if (it != devices_.end()) {
    devices_.erase(it);
  }
}

bool Bus::read(u32 addr, u32 size, u32 &value) {
  // Check devices first
  for (Device *device : devices_) {
    if (addr >= device->base_addr() &&
        addr < device->base_addr() + device->size()) {
      u32 offset = addr - device->base_addr();
      if (device->read(offset, size, value)) {
        return true;
      }
    }
  }

  // Check memory
  if (memory_) {
    switch (size) {
    case 1:
      value = memory_->read8(addr);
      return true;
    case 2:
      value = memory_->read16(addr);
      return true;
    case 4:
      value = memory_->read32(addr);
      return true;
    case 8:
      value = static_cast<u32>(memory_->read64(addr));
      return true;
    }
  }

  // Unmapped address
  O2EMU_LOG_WARN("Bus read from unmapped address: 0x" << std::hex << addr
                                                      << std::dec);
  value = 0xFFFFFFFF;
  return false;
}

bool Bus::write(u32 addr, u32 size, u32 value) {
  // Check devices first
  for (Device *device : devices_) {
    if (addr >= device->base_addr() &&
        addr < device->base_addr() + device->size()) {
      u32 offset = addr - device->base_addr();
      if (device->write(offset, size, value)) {
        return true;
      }
    }
  }

  // Check memory
  if (memory_) {
    switch (size) {
    case 1:
      memory_->write8(addr, value);
      return true;
    case 2:
      memory_->write16(addr, value);
      return true;
    case 4:
      memory_->write32(addr, value);
      return true;
    case 8:
      memory_->write64(addr, value);
      return true;
    }
  }

  // Unmapped address
  O2EMU_LOG_WARN("Bus write to unmapped address: 0x"
                 << std::hex << addr << " value=0x" << value << std::dec);
  return false;
}

void Bus::tick(u64 cycles) {
  for (Device *device : devices_) {
    device->tick(cycles);
  }
}

void Bus::reset() {
  for (Device *device : devices_) {
    device->reset();
  }
}

} // namespace o2emu::system