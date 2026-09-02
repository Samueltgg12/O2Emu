#include "o2emu/bus.h"
#include "o2emu/logging.h"
#include "o2emu/tracing.h"
#include <algorithm>

namespace o2emu {

void SystemBus::add_device(std::unique_ptr<BusDevice> device) {
  std::lock_guard<std::mutex> lock(mutex_);
  DeviceEntry entry;
  entry.base = device->base_address();
  entry.size = device->size();
  entry.device = std::move(device);
  devices_.push_back(std::move(entry));
  // Sort by base address for efficient lookup
  std::sort(devices_.begin(), devices_.end(),
            [](const DeviceEntry &a, const DeviceEntry &b) {
              return a.base < b.base;
            });
  O2E_INFO(LogCategory::Bus, "Added device {} at 0x{:08x} size 0x{:x}",
           devices_.back().device->name(), devices_.back().base,
           devices_.back().size);
}

void SystemBus::remove_device(uint32_t base_address) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = std::find_if(
      devices_.begin(), devices_.end(),
      [base_address](const DeviceEntry &e) { return e.base == base_address; });
  if (it != devices_.end()) {
    O2E_INFO(LogCategory::Bus, "Removed device {} at 0x{:08x}",
             it->device->name(), it->base);
    devices_.erase(it);
  }
}

BusDevice *SystemBus::find_device(uint32_t addr) {
  std::lock_guard<std::mutex> lock(mutex_);
  // Binary search since devices_ is sorted by base
  auto it = std::upper_bound(
      devices_.begin(), devices_.end(), addr,
      [](uint32_t value, const DeviceEntry &e) { return value < e.base; });
  if (it != devices_.begin()) {
    --it;
    if (addr >= it->base && addr < it->base + it->size) {
      return it->device.get();
    }
  }
  return nullptr;
}

uint8_t SystemBus::read8(uint32_t addr) {
  BusDevice *dev = find_device(addr);
  if (dev) {
    uint32_t offset = addr - dev->base_address();
    uint8_t val = dev->read8(offset);
    O2E_LOG_READ(Bus, dev->name() + "_read8", val);
    Tracer::instance().trace_memory_access(
        {0, addr, 1, false, val, 0, dev->name()});
    return val;
  }
  O2E_WARN(LogCategory::Bus, "Read8 from unmapped address 0x{:08x}", addr);
  return 0;
}

uint16_t SystemBus::read16(uint32_t addr) {
  BusDevice *dev = find_device(addr);
  if (dev) {
    uint32_t offset = addr - dev->base_address();
    uint16_t val = dev->read16(offset);
    O2E_LOG_READ(Bus, dev->name() + "_read16", val);
    Tracer::instance().trace_memory_access(
        {0, addr, 2, false, val, 0, dev->name()});
    return val;
  }
  O2E_WARN(LogCategory::Bus, "Read16 from unmapped address 0x{:08x}", addr);
  return 0;
}

uint32_t SystemBus::read32(uint32_t addr) {
  BusDevice *dev = find_device(addr);
  if (dev) {
    uint32_t offset = addr - dev->base_address();
    uint32_t val = dev->read32(offset);
    O2E_LOG_READ(Bus, dev->name() + "_read32", val);
    Tracer::instance().trace_memory_access(
        {0, addr, 4, false, val, 0, dev->name()});
    return val;
  }
  O2E_WARN(LogCategory::Bus, "Read32 from unmapped address 0x{:08x}", addr);
  return 0;
}

uint64_t SystemBus::read64(uint32_t addr) {
  BusDevice *dev = find_device(addr);
  if (dev) {
    uint32_t offset = addr - dev->base_address();
    uint64_t val = dev->read64(offset);
    O2E_LOG_READ(Bus, dev->name() + "_read64", val);
    Tracer::instance().trace_memory_access(
        {0, addr, 8, false, val, 0, dev->name()});
    return val;
  }
  O2E_WARN(LogCategory::Bus, "Read64 from unmapped address 0x{:08x}", addr);
  return 0;
}

void SystemBus::write8(uint32_t addr, uint8_t val) {
  BusDevice *dev = find_device(addr);
  if (dev) {
    uint32_t offset = addr - dev->base_address();
    dev->write8(offset, val);
    O2E_LOG_WRITE(Bus, dev->name() + "_write8", val);
    Tracer::instance().trace_memory_access(
        {0, addr, 1, true, val, 0, dev->name()});
    return;
  }
  O2E_WARN(LogCategory::Bus, "Write8 to unmapped address 0x{:08x} = 0x{:02x}",
           addr, val);
}

void SystemBus::write16(uint32_t addr, uint16_t val) {
  BusDevice *dev = find_device(addr);
  if (dev) {
    uint32_t offset = addr - dev->base_address();
    dev->write16(offset, val);
    O2E_LOG_WRITE(Bus, dev->name() + "_write16", val);
    Tracer::instance().trace_memory_access(
        {0, addr, 2, true, val, 0, dev->name()});
    return;
  }
  O2E_WARN(LogCategory::Bus, "Write16 to unmapped address 0x{:08x} = 0x{:04x}",
           addr, val);
}

void SystemBus::write32(uint32_t addr, uint32_t val) {
  BusDevice *dev = find_device(addr);
  if (dev) {
    uint32_t offset = addr - dev->base_address();
    dev->write32(offset, val);
    O2E_LOG_WRITE(Bus, dev->name() + "_write32", val);
    Tracer::instance().trace_memory_access(
        {0, addr, 4, true, val, 0, dev->name()});
    return;
  }
  O2E_WARN(LogCategory::Bus, "Write32 to unmapped address 0x{:08x} = 0x{:08x}",
           addr, val);
}

void SystemBus::write64(uint32_t addr, uint64_t val) {
  BusDevice *dev = find_device(addr);
  if (dev) {
    uint32_t offset = addr - dev->base_address();
    dev->write64(offset, val);
    O2E_LOG_WRITE(Bus, dev->name() + "_write64", val);
    Tracer::instance().trace_memory_access(
        {0, addr, 8, true, val, 0, dev->name()});
    return;
  }
  O2E_WARN(LogCategory::Bus, "Write64 to unmapped address 0x{:08x} = 0x{:016x}",
           addr, val);
}

void SystemBus::reset_all() {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto &entry : devices_) {
    entry.device->reset();
  }
  O2E_INFO(LogCategory::Bus, "All devices reset");
}

} // namespace o2emu