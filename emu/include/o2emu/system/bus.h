#pragma once

/**
 * @file bus.h
 * @brief System bus interconnect
 */

#include <memory>
#include <o2emu/devices/device.h>
#include <o2emu/memory/memory.h>
#include <o2emu/o2emu.h>
#include <vector>

namespace o2emu::system {

class Bus {
public:
  Bus();
  ~Bus() = default;

  // Attach a device to the bus
  void attach_device(std::unique_ptr<devices::Device> device);

  // Attach memory to the bus
  void attach_memory(o2emu::memory::Memory *memory);

  // Translate virtual address to physical address for direct-mapped MIPS
  // segments (KSEG0 0x80000000..0x9FFFFFFF and KSEG1 0xA0000000..0xBFFFFFFF)
  static constexpr u32 to_physical(u32 addr) {
    if (addr >= 0x80000000 && addr < 0xC0000000) {
      return addr & 0x1FFFFFFF;
    }
    return addr;
  }

  // Find device at physical address
  devices::Device *find_device(u32 phys_addr) const;

  // Bus read/write
  u64 read64(u32 phys_addr);
  u32 read32(u32 phys_addr);
  u16 read16(u32 phys_addr);
  u8 read8(u32 phys_addr);

  void write64(u32 phys_addr, u64 value);
  void write32(u32 phys_addr, u32 value);
  void write16(u32 phys_addr, u16 value);
  void write8(u32 phys_addr, u8 value);

  // Reset all devices
  void reset();

  // Tick all devices
  void tick(u64 cycles);

private:
  struct DeviceEntry {
    u32 base;
    u32 size;
    devices::Device *device;
  };
  std::vector<DeviceEntry> devices_;
  std::vector<std::unique_ptr<devices::Device>> owned_devices_;
  o2emu::memory::Memory *memory_ = nullptr;
};

} // namespace o2emu::system