#pragma once

#include "memory_map.h"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace o2emu {

// Bus device interface
class BusDevice {
public:
  virtual ~BusDevice() = default;

  virtual uint32_t base_address() const = 0;
  virtual uint32_t size() const = 0;
  virtual std::string name() const = 0;

  // Read/write callbacks
  virtual uint8_t read8(uint32_t offset) = 0;
  virtual uint16_t read16(uint32_t offset) = 0;
  virtual uint32_t read32(uint32_t offset) = 0;
  virtual uint64_t read64(uint32_t offset) = 0;

  virtual void write8(uint32_t offset, uint8_t val) = 0;
  virtual void write16(uint32_t offset, uint16_t val) = 0;
  virtual void write32(uint32_t offset, uint32_t val) = 0;
  virtual void write64(uint32_t offset, uint64_t val) = 0;

  // Optional: reset device state
  virtual void reset() {}
};

// System bus - manages address decoding and device routing
class SystemBus {
public:
  SystemBus() = default;

  void add_device(std::unique_ptr<BusDevice> device);
  void remove_device(uint32_t base_address);

  // Memory access
  uint8_t read8(uint32_t addr);
  uint16_t read16(uint32_t addr);
  uint32_t read32(uint32_t addr);
  uint64_t read64(uint32_t addr);

  void write8(uint32_t addr, uint8_t val);
  void write16(uint32_t addr, uint16_t val);
  void write32(uint32_t addr, uint32_t val);
  void write64(uint32_t addr, uint64_t val);

  // Find device at address
  BusDevice *find_device(uint32_t addr);

  // Reset all devices
  void reset_all();

private:
  struct DeviceEntry {
    uint32_t base;
    uint32_t size;
    std::unique_ptr<BusDevice> device;
  };

  std::vector<DeviceEntry> devices_;
  std::mutex mutex_;
};

// DMA controller interface
class DmaController {
public:
  virtual ~DmaController() = default;

  enum class Direction { Read, Write };
  enum class Status { Idle, Active, Done, Error };

  virtual bool start_transfer(uint32_t src_addr, uint32_t dst_addr,
                              uint32_t size, Direction dir) = 0;
  virtual Status status() const = 0;
  virtual void cancel() = 0;
};

// Interrupt controller interface
class InterruptController {
public:
  virtual ~InterruptController() = default;

  virtual void raise_interrupt(uint32_t irq) = 0;
  virtual void lower_interrupt(uint32_t irq) = 0;
  virtual uint32_t pending_interrupts() const = 0;
  virtual int highest_priority_irq() const = 0;
  virtual void ack_interrupt(uint32_t irq) = 0;
};

} // namespace o2emu