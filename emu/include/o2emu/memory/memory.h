#pragma once

/**
 * @file memory.h
 * @brief Memory system interface
 */

#include <functional>
#include <memory>
#include <o2emu/o2emu.h>
#include <vector>

namespace o2emu::memory {

class AddressSpace;
class CRIME;
class MRE;

enum class MemoryRegionType {
  RAM,
  ROM,
  MMIO,
  UNMAPPED,
};

struct MemoryRegion {
  u32 base;
  u32 size;
  MemoryRegionType type;
  bool readable = true;
  bool writable = true;
  bool executable = true;
  std::string name;
};

class Memory {
public:
  Memory();
  ~Memory();

  // Non-copyable, movable
  Memory(const Memory &) = delete;
  Memory &operator=(const Memory &) = delete;
  Memory(Memory &&) = default;
  Memory &operator=(Memory &&) = default;

  // Initialize memory system
  bool init(u32 ram_size_mb = 256);

  // Memory access (physical addresses)
  u64 read64(u32 addr) const;
  u32 read32(u32 addr) const;
  u16 read16(u32 addr) const;
  u8 read8(u32 addr) const;

  void write64(u32 addr, u64 value);
  void write32(u32 addr, u32 value);
  void write16(u32 addr, u16 value);
  void write8(u32 addr, u8 value);

  // Block operations
  void read_block(u32 addr, void *buffer, size_t size);
  void write_block(u32 addr, const void *buffer, size_t size);

  // Memory mapping
  void map_region(const MemoryRegion &region);
  void unmap_region(u32 base);

  // Direct RAM access (for DMA, etc.)
  u8 *get_ram_ptr(u32 phys_addr);
  const u8 *get_ram_ptr(u32 phys_addr) const;

  // RAM size
  u32 ram_size() const { return ram_size_; }
  void set_ram_size(u32 size_mb);

  // Utility functions
  void clear();
  const u8 *data() const;
  u8 *data();
  u32 size() const;

  // CRIME memory controller
  CRIME &crime() { return *crime_; }
  const CRIME &crime() const { return *crime_; }

  // MRE
  MRE &mre() { return *mre_; }
  const MRE &mre() const { return *mre_; }

  // Address space
  AddressSpace &address_space() { return *address_space_; }
  const AddressSpace &address_space() const { return *address_space_; }

  // Reset
  void reset();

private:
  u32 ram_size_ = 0;
  u32 ram_mask_ = 0;
  std::unique_ptr<u8[]> ram_;

  std::unique_ptr<AddressSpace> address_space_;
  std::unique_ptr<CRIME> crime_;
  std::unique_ptr<MRE> mre_;

  std::vector<MemoryRegion> regions_;
};

} // namespace o2emu::memory