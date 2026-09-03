/**
 * @file address_space.cpp
 * @brief Address space mapping for IP32
 */

#include <algorithm>
#include <o2emu/logging/logger.h>
#include <o2emu/memory/address_space.h>

namespace o2emu::memory {

AddressSpace::AddressSpace() {
  // Initialize with default mappings
  reset();
}

AddressSpace::~AddressSpace() = default;

void AddressSpace::reset() {
  mappings_.clear();

  // IP32 Physical Memory Map (from docs/cpu-memory.md)
  // 0x0000_0000 - 0x0FFF_FFFF : Main Memory (0-256MB)
  // 0x1000_0000 - 0x1FFF_FFFF : Main Memory (256-512MB)
  // 0x2000_0000 - 0x2FFF_FFFF : Main Memory (512-768MB)
  // 0x3000_0000 - 0x3FFF_FFFF : Main Memory (768-1024MB)
  // 0x4000_0000 - 0x4FFF_FFFF : CRIME registers
  // 0x5000_0000 - 0x5FFF_FFFF : MRE registers
  // 0x6000_0000 - 0x6FFF_FFFF : Display Engine registers
  // 0x7000_0000 - 0x7FFF_FFFF : MACE registers
  // 0x8000_0000 - 0x8FFF_FFFF : PROM (cached)
  // 0x9000_0000 - 0x9FFF_FFFF : PROM (uncached)
  // 0xA000_0000 - 0xAFFF_FFFF : PCI Memory Space
  // 0xB000_0000 - 0xBFFF_FFFF : PCI I/O Space
  // 0xC000_0000 - 0xCFFF_FFFF : ISA I/O Space
  // 0xD000_0000 - 0xDFFF_FFFF : ISA Memory Space
  // 0xE000_0000 - 0xEFFF_FFFF : Reserved
  // 0xF000_0000 - 0xFFFF_FFFF : Reserved

  // These will be populated by the device drivers
}

void AddressSpace::map(u32 base, u32 size, Device *device, u32 offset) {
  Mapping mapping;
  mapping.base = base;
  mapping.size = size;
  mapping.device = device;
  mapping.offset = offset;
  mapping.read_only = false;

  mappings_.push_back(mapping);

  // Sort by base address for efficient lookup
  std::sort(mappings_.begin(), mappings_.end(),
            [](const Mapping &a, const Mapping &b) { return a.base < b.base; });

  O2EMU_LOG_DEBUG("Mapped device at 0x" << std::hex << base << " size 0x"
                                        << size << std::dec);
}

void AddressSpace::unmap(u32 base, u32 size) {
  auto it = std::remove_if(mappings_.begin(), mappings_.end(),
                           [base, size](const Mapping &m) {
                             return m.base == base && m.size == size;
                           });
  mappings_.erase(it, mappings_.end());
}

AddressSpace::Device *AddressSpace::find_device(u32 addr) const {
  for (const auto &mapping : mappings_) {
    if (addr >= mapping.base && addr < mapping.base + mapping.size) {
      return mapping.device;
    }
  }
  return nullptr;
}

u32 AddressSpace::translate(u32 addr, u32 &offset) const {
  for (const auto &mapping : mappings_) {
    if (addr >= mapping.base && addr < mapping.base + mapping.size) {
      offset = mapping.offset + (addr - mapping.base);
      return mapping.base;
    }
  }
  offset = 0;
  return 0;
}

bool AddressSpace::is_mapped(u32 addr) const {
  return find_device(addr) != nullptr;
}

bool AddressSpace::is_read_only(u32 addr) const {
  for (const auto &mapping : mappings_) {
    if (addr >= mapping.base && addr < mapping.base + mapping.size) {
      return mapping.read_only;
    }
  }
  return false;
}

} // namespace o2emu::memory