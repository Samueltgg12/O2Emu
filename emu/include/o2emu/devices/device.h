#pragma once

/**
 * @file device.h
 * @brief Base device interface
 */

#include <o2emu/o2emu.h>
#include <string>

namespace o2emu::devices {

class Device {
public:
  Device(const std::string &name, u32 base_addr, u32 size);
  virtual ~Device() = default;

  // Non-copyable
  Device(const Device &) = delete;
  Device &operator=(const Device &) = delete;

  // Device identification
  const std::string &name() const { return name_; }
  u32 base_addr() const { return base_addr_; }
  u32 size() const { return size_; }

  // Memory-mapped I/O access
  virtual u32 read32(u32 offset) = 0;
  virtual u16 read16(u32 offset) = 0;
  virtual u8 read8(u32 offset) = 0;

  virtual void write32(u32 offset, u32 value) = 0;
  virtual void write16(u32 offset, u16 value) = 0;
  virtual void write8(u32 offset, u8 value) = 0;

  // Register access (for devices with register arrays)
  virtual u32 read_reg(u32 offset) { return read32(offset); }
  virtual void write_reg(u32 offset, u32 value) { write32(offset, value); }

  // Reset device state
  virtual void reset() = 0;

  // Optional: timer tick for devices that need periodic updates
  virtual void tick([[maybe_unused]] u64 cycles) {}

  // Optional: interrupt handling
  virtual void set_interrupt_line([[maybe_unused]] int line,
                                  [[maybe_unused]] bool asserted) {}

protected:
  std::string name_;
  u32 base_addr_;
  u32 size_;
};

} // namespace o2emu::devices