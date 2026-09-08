/**
 * @file memory.cpp
 * @brief Physical memory implementation
 */

#include <algorithm>
#include <cstring>
#include <memory>
#include <new>
#include <o2emu/logging/logger.h>
#include <o2emu/memory/address_space.h>
#include <o2emu/memory/crime.h>
#include <o2emu/memory/memory.h>
#include <o2emu/memory/mre.h>

namespace o2emu::memory {

Memory::Memory() : ram_size_(0), ram_mask_(0), ram_(nullptr) {}

Memory::~Memory() = default;

bool Memory::init(u32 size_mb) {
  if (size_mb == 0 || size_mb > 1024) {
    O2EMU_LOG_ERROR_F("Invalid RAM size: {} MB", size_mb);
    return false;
  }

  ram_size_ = size_mb * 1024 * 1024;
  ram_mask_ = ram_size_ - 1;

  // Allocate RAM (aligned to 16 bytes for SIMD)
  ram_.reset(new (std::align_val_t(16)) u8[ram_size_]);
  if (!ram_) {
    O2EMU_LOG_ERROR_F("Failed to allocate {} MB RAM", size_mb);
    return false;
  }

  // Clear RAM
  std::memset(ram_.get(), 0, ram_size_);

  O2EMU_LOG_INFO_F("Initialized {} MB RAM ({} bytes)", size_mb, ram_size_);
  return true;
}

u8 Memory::read8(u32 addr) const {
  addr &= ram_mask_;
  return ram_[addr];
}

u16 Memory::read16(u32 addr) const {
  addr &= ram_mask_;
  return static_cast<u16>((ram_[addr] << 8) | ram_[(addr + 1) & ram_mask_]);
}

u32 Memory::read32(u32 addr) const {
  addr &= ram_mask_;
  return (static_cast<u32>(ram_[addr]) << 24) |
         (static_cast<u32>(ram_[(addr + 1) & ram_mask_]) << 16) |
         (static_cast<u32>(ram_[(addr + 2) & ram_mask_]) << 8) |
         static_cast<u32>(ram_[(addr + 3) & ram_mask_]);
}

u64 Memory::read64(u32 addr) const {
  const u32 hi = read32(addr);
  const u32 lo = read32((addr + 4) & ram_mask_);
  return (static_cast<u64>(hi) << 32) | lo;
}

void Memory::write8(u32 addr, u8 value) {
  addr &= ram_mask_;
  ram_[addr] = value;
}

void Memory::write16(u32 addr, u16 value) {
  addr &= ram_mask_;
  ram_[addr] = static_cast<u8>(value >> 8);
  ram_[(addr + 1) & ram_mask_] = static_cast<u8>(value);
}

void Memory::write32(u32 addr, u32 value) {
  addr &= ram_mask_;
  ram_[addr] = static_cast<u8>(value >> 24);
  ram_[(addr + 1) & ram_mask_] = static_cast<u8>(value >> 16);
  ram_[(addr + 2) & ram_mask_] = static_cast<u8>(value >> 8);
  ram_[(addr + 3) & ram_mask_] = static_cast<u8>(value);
}

void Memory::write64(u32 addr, u64 value) {
  write32(addr, static_cast<u32>(value >> 32));
  write32((addr + 4) & ram_mask_, static_cast<u32>(value));
}

void Memory::read_block(u32 addr, void *buffer, size_t size) {
  u8 *dst = static_cast<u8 *>(buffer);
  for (size_t i = 0; i < size; ++i) {
    dst[i] = read8(addr + i);
  }
}

void Memory::write_block(u32 addr, const void *buffer, size_t size) {
  const u8 *src = static_cast<const u8 *>(buffer);
  for (size_t i = 0; i < size; ++i) {
    write8(addr + i, src[i]);
  }
}

void Memory::clear() {
  if (ram_) {
    std::memset(ram_.get(), 0, ram_size_);
  }
}

const u8 *Memory::data() const { return ram_.get(); }

u8 *Memory::data() { return ram_.get(); }

u32 Memory::size() const { return ram_size_; }

} // namespace o2emu::memory