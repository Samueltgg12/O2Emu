#include "o2emu/memory_map.h"
#include "o2emu/logging.h"
#include "o2emu/tracing.h"
#include <cstring>

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

  void write8(uint32_t offset, uint8_t) override { /* read-only */ }
  void write16(uint32_t offset, uint16_t) override { /* read-only */ }
  void write32(uint32_t offset, uint32_t) override { /* read-only */ }
  void write64(uint32_t offset, uint64_t) override { /* read-only */ }

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

} // namespace o2emu