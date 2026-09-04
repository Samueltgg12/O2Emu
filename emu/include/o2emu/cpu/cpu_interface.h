/**
 * @file cpu_interface.h
 * @brief Common CPU interface for different MIPS implementations
 */

#pragma once

#include <functional>
#include <memory>
#include <o2emu/cpu/cpu.h>
#include <o2emu/o2emu.h>

namespace o2emu::cpu {

// ExceptionCode and InterruptLine are defined in cpu.h

enum class CPUType { R5000, R10000, R12000 };

class ICpu {
public:
  virtual ~ICpu() = default;

  // Initialize CPU state
  virtual void reset(uint32_t reset_vector = 0xBFC00000) = 0;

  // Execute instructions
  virtual void step() = 0;
  virtual void run(uint64_t cycles) = 0;
  virtual void run_until(uint32_t target_pc) = 0;

  // Memory access callbacks
  using ReadCallback = std::function<uint32_t(uint32_t addr, uint32_t size)>;
  using WriteCallback =
      std::function<void(uint32_t addr, uint32_t size, uint32_t value)>;

  virtual void set_memory_read_callback(ReadCallback cb) = 0;
  virtual void set_memory_write_callback(WriteCallback cb) = 0;

  // Interrupt handling
  virtual void raise_interrupt(InterruptLine line) = 0;
  virtual void clear_interrupt(InterruptLine line) = 0;

  // State access
  virtual uint32_t gpr(int reg) const = 0;
  virtual void set_gpr(int reg, uint32_t value) = 0;
  virtual uint64_t gpr64(int reg) const = 0;
  virtual void set_gpr64(int reg, uint64_t value) = 0;

  virtual uint32_t pc() const = 0;
  virtual void set_pc(uint32_t pc) = 0;

  virtual uint32_t cp0_reg(int reg) const = 0;
  virtual void set_cp0_reg(int reg, uint32_t value) = 0;

  virtual uint64_t cycles() const = 0;
  virtual uint64_t instructions() const = 0;

  // Debugging
  virtual void dump_registers() const = 0;
  virtual void disassemble(uint32_t addr, char *buffer, size_t size) const = 0;

  // CPU type identification
  virtual CPUType type() const = 0;
  virtual const char *type_name() const = 0;
};

// Factory function
std::unique_ptr<ICpu> create_cpu(CPUType type,
                                 o2emu::system::Bus *bus = nullptr);

} // namespace o2emu::cpu