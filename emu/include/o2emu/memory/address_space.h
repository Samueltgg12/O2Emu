#pragma once

/**
 * @file address_space.h
 * @brief Address space mapping and translation
 */

#include <o2emu/o2emu.h>
#include <vector>

namespace o2emu::memory {

class AddressSpace {
public:
  // Abstract base class for memory-mapped devices
  class Device {
  public:
    virtual ~Device() = default;
    virtual u32 read(u32 offset) = 0;
    virtual void write(u32 offset, u32 value) = 0;
  };

  struct Mapping {
    u32 base;
    u32 size;
    Device *device = nullptr;
    u32 offset = 0;
    bool read_only = false;
  };

  AddressSpace();
  ~AddressSpace() = default;

  // Reset address space to default state
  void reset();

  // Map a device at a virtual address range with device-relative offset
  void map(u32 base, u32 size, Device *device, u32 offset = 0);

  // Unmap a range
  void unmap(u32 base, u32 size);

  // Find device at address
  Device *find_device(u32 addr) const;

  // Translate virtual address to device offset
  // Returns the mapping's base address, sets offset to device-relative offset
  u32 translate(u32 addr, u32 &offset) const;

  // Check if address is mapped
  bool is_mapped(u32 addr) const;

  // Check if address is read-only
  bool is_read_only(u32 addr) const;

private:
  std::vector<Mapping> mappings_;
};

} // namespace o2emu::memory