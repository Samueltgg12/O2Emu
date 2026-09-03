#pragma once

/**
 * @file address_space.h
 * @brief Address space mapping and translation
 */

#include <functional>
#include <memory>
#include <o2emu/o2emu.h>
#include <vector>

namespace o2emu::memory {

class Device;

struct Mapping {
  u32 phys_base;
  u32 size;
  Device *device = nullptr;
  bool readable = true;
  bool writable = true;
  std::string name;
};

class AddressSpace {
public:
  AddressSpace();
  ~AddressSpace() = default;

  // Map a device at a physical address range
  void map_device(u32 phys_base, u32 size, Device *device,
                  const char *name = "");

  // Unmap a range
  void unmap(u32 phys_base, u32 size);

  // Find device at address
  Device *find_device(u32 phys_addr) const;

  // Read/write through address space
  u32 read32(u32 phys_addr);
  u16 read16(u32 phys_addr);
  u8 read8(u32 phys_addr);

  void write32(u32 phys_addr, u32 value);
  void write16(u32 phys_addr, u16 value);
  void write8(u32 phys_addr, u8 value);

  // Debug
  void dump_mappings() const;

private:
  std::vector<Mapping> mappings_;

  // Find mapping index for address
  int find_mapping(u32 phys_addr) const;
};

} // namespace o2emu::memory