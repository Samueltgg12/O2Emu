#include "o2emu/memory_map.h"
#include "o2emu/bus.h"
#include "o2emu/logging.h"
#include "o2emu/tracing.h"
#include <algorithm>
#include <cstring>
#include <fstream>

namespace o2emu {

// Simple RAM implementation for testing
class RamDevice : public BusDevice {
public:
  RamDevice(uint32_t base, uint32_t size, const std::string &name)
      : base_(base), size_(size), name_(name), data_(size, 0) {}

  uint32_t base_address() const override { return base_; }
  uint32_t size() const override { return size_; }
  std::string name() const override { return name_; }

  uint8_t read8(uint32_t offset) override {
    if (offset >= size_)
      return 0;
    return data_[offset];
  }

  uint16_t read16(uint32_t offset) override {
    if (offset + 1 >= size_)
      return 0;
    return *reinterpret_cast<uint16_t *>(&data_[offset]);
  }

  uint32_t read32(uint32_t offset) override {
    if (offset + 3 >= size_)
      return 0;
    return *reinterpret_cast<uint32_t *>(&data_[offset]);
  }

  uint64_t read64(uint32_t offset) override {
    if (offset + 7 >= size_)
      return 0;
    return *reinterpret_cast<uint64_t *>(&data_[offset]);
  }

  void write8(uint32_t offset, uint8_t val) override {
    if (offset >= size_)
      return;
    data_[offset] = val;
  }

  void write16(uint32_t offset, uint16_t val) override {
    if (offset + 1 >= size_)
      return;
    *reinterpret_cast<uint16_t *>(&data_[offset]) = val;
  }

  void write32(uint32_t offset, uint32_t val) override {
    if (offset + 3 >= size_)
      return;
    *reinterpret_cast<uint32_t *>(&data_[offset]) = val;
  }

  void write64(uint32_t offset, uint64_t val) override {
    if (offset + 7 >= size_)
      return;
    *reinterpret_cast<uint64_t *>(&data_[offset]) = val;
  }

  void reset() override { std::fill(data_.begin(), data_.end(), 0); }

  // Direct memory access for loading firmware
  std::vector<uint8_t> &data() { return data_; }
  const std::vector<uint8_t> &data() const { return data_; }

private:
  uint32_t base_;
  uint32_t size_;
  std::string name_;
  std::vector<uint8_t> data_;
};

// PROM device (read-only)
class PromDevice : public BusDevice {
public:
  PromDevice(uint32_t base, uint32_t size, const std::string &name)
      : base_(base), size_(size), name_(name), data_(size, 0) {}

  uint32_t base_address() const override { return base_; }
  uint32_t size() const override { return size_; }
  std::string name() const override { return name_; }

  uint8_t read8(uint32_t offset) override {
    if (offset >= size_)
      return 0;
    return data_[offset];
  }

  uint16_t read16(uint32_t offset) override {
    if (offset + 1 >= size_)
      return 0;
    return *reinterpret_cast<uint16_t *>(&data_[offset]);
  }

  uint32_t read32(uint32_t offset) override {
    if (offset + 3 >= size_)
      return 0;
    return *reinterpret_cast<uint32_t *>(&data_[offset]);
  }

  uint64_t read64(uint32_t offset) override {
    if (offset + 7 >= size_)
      return 0;
    return *reinterpret_cast<uint64_t *>(&data_[offset]);
  }

  void write8(uint32_t offset, uint8_t) override { (void)offset; } // read-only
  void write16(uint32_t offset, uint16_t) override {
    (void)offset;
  } // read-only
  void write32(uint32_t offset, uint32_t) override {
    (void)offset;
  } // read-only
  void write64(uint32_t offset, uint64_t) override {
    (void)offset;
  } // read-only

  void load_from_file(const std::string &path) {
    std::ifstream file(path, std::ios::binary);
    if (file) {
      file.read(reinterpret_cast<char *>(data_.data()), size_);
    }
  }

  std::vector<uint8_t> &data() { return data_; }
  const std::vector<uint8_t> &data() const { return data_; }

private:
  uint32_t base_;
  uint32_t size_;
  std::string name_;
  std::vector<uint8_t> data_;
};

LinearMemoryMap::LinearMemoryMap(uint32_t size) : size_(size), data_(size, 0) {}

uint32_t LinearMemoryMap::size() const { return size_; }

uint8_t LinearMemoryMap::read8(uint32_t addr) {
  if (addr >= size_) {
    return 0;
  }
  return data_[addr];
}

uint16_t LinearMemoryMap::read16(uint32_t addr) {
  if (addr + sizeof(uint16_t) > size_) {
    return 0;
  }
  return static_cast<uint16_t>(data_[addr] |
                               (static_cast<uint16_t>(data_[addr + 1]) << 8));
}

uint32_t LinearMemoryMap::read32(uint32_t addr) {
  if (addr + sizeof(uint32_t) > size_) {
    return 0;
  }
  return static_cast<uint32_t>(data_[addr]) |
         (static_cast<uint32_t>(data_[addr + 1]) << 8) |
         (static_cast<uint32_t>(data_[addr + 2]) << 16) |
         (static_cast<uint32_t>(data_[addr + 3]) << 24);
}

uint64_t LinearMemoryMap::read64(uint32_t addr) {
  if (addr + sizeof(uint64_t) > size_) {
    return 0;
  }
  uint64_t value = 0;
  for (uint32_t i = 0; i < sizeof(uint64_t); ++i) {
    value |= (static_cast<uint64_t>(data_[addr + i]) << (i * 8u));
  }
  return value;
}

void LinearMemoryMap::write8(uint32_t addr, uint8_t val) {
  if (addr >= size_) {
    return;
  }
  data_[addr] = val;
}

void LinearMemoryMap::write16(uint32_t addr, uint16_t val) {
  if (addr + sizeof(uint16_t) > size_) {
    return;
  }
  data_[addr] = static_cast<uint8_t>(val & 0xffu);
  data_[addr + 1] = static_cast<uint8_t>((val >> 8) & 0xffu);
}

void LinearMemoryMap::write32(uint32_t addr, uint32_t val) {
  if (addr + sizeof(uint32_t) > size_) {
    return;
  }
  data_[addr] = static_cast<uint8_t>(val & 0xffu);
  data_[addr + 1] = static_cast<uint8_t>((val >> 8) & 0xffu);
  data_[addr + 2] = static_cast<uint8_t>((val >> 16) & 0xffu);
  data_[addr + 3] = static_cast<uint8_t>((val >> 24) & 0xffu);
}

void LinearMemoryMap::write64(uint32_t addr, uint64_t val) {
  if (addr + sizeof(uint64_t) > size_) {
    return;
  }
  for (uint32_t i = 0; i < sizeof(uint64_t); ++i) {
    data_[addr + i] = static_cast<uint8_t>((val >> (i * 8u)) & 0xffu);
  }
}

void LinearMemoryMap::reset() { std::fill(data_.begin(), data_.end(), 0); }

void LinearMemoryMap::load_bytes(const std::vector<uint8_t> &bytes,
                                 uint32_t offset) {
  if (offset >= size_) {
    return;
  }
  const uint32_t max = static_cast<uint32_t>(bytes.size());
  const uint32_t remaining = size_ - offset;
  const uint32_t count = std::min(max, remaining);
  for (uint32_t i = 0; i < count; ++i) {
    data_[offset + i] = bytes[i];
  }
}

std::unique_ptr<MemoryMap> create_ram_map(uint32_t size) {
  return std::make_unique<LinearMemoryMap>(size);
}

std::unique_ptr<MemoryMap> create_prom_map(uint32_t size) {
  return std::make_unique<LinearMemoryMap>(size);
}

} // namespace o2emu