#pragma once

/**
 * @file bus.h
 * @brief System bus interconnect
 */

#include <memory>
#include <o2emu/devices/device.h>
#include <o2emu/o2emu.h>
#include <vector>

namespace o2emu::system {

class Bus {
public:
  Bus();
  ~Bus() = default;

  // Attach a device to the bus
  void attach_device(std::unique_ptr<devices::Device> device);

  // Find device at physical address
  devices::Device *find_device(u32 phys_addr) const;

  // Bus read/write
  u32 read32(u32 phys_addr);
  u16 read16(u32 phys_addr);
  u8 read8(u32 phys_addr);

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
};

} // namespace o2emu::system