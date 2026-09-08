#pragma once

/**
 * @file cpu.h
 * @brief MIPS CPU core interface
 */

#include "cp0.h"
#include <functional>
#include <memory>
#include <o2emu/o2emu.h>
namespace o2emu::cpu {

// Forward declarations
class CP0;
class MMU;
class Cache;

enum class ExceptionCode : uint32_t {
  INT = 0,  // External interrupt
  MOD = 1,  // TLB modification
  TLBL = 2, // TLB load/instruction fetch miss
  TLBS = 3, // TLB store miss
  ADEL = 4, // Address error load/instruction fetch
  ADES = 5, // Address error store
  IBE = 6,  // Bus error instruction fetch
  DBE = 7,  // Bus error data load/store
  SYS = 8,  // Syscall
  BP = 9,   // Breakpoint
  RI = 10,  // Reserved instruction
  CPU = 11, // Coprocessor unusable
  OV = 12,  // Arithmetic overflow
  TR = 13,  // Trap
  FPE = 15, // Floating point exception
            // ... more as needed
};

// Alias for backward compatibility with code using Exception:: prefix
using Exception = ExceptionCode;

enum class InterruptLine : uint32_t {
  INT0 = 0,
  INT1 = 1,
  INT2 = 2,
  INT3 = 3,
  INT4 = 4,
  INT5 = 5,
  INT6 = 6,
  INT7 = 7,
};

struct CPUState {
  // General purpose registers
  u64 gpr[32] = {}; // $0-$31

  // Special registers
  u32 pc = 0;      // Program counter
  u32 next_pc = 0; // Next PC (for branch delay slots)
  u64 hi = 0;      // Multiply/divide high
  u64 lo = 0;      // Multiply/divide low

  // FPU registers (32 single-precision or 16 double-precision)
  union {
    float fpr_s[32];
    double fpr_d[16];
    u32 fpr_u[32];
    u64 fpr[32]; // 64-bit view for double-precision
  };
  u32 fcr0 = 0;  // FPU control/status
  u32 fcr31 = 0; // FPU control/status register

  // Execution state
  bool in_delay_slot = false;
  u32 delay_slot_pc = 0;
  bool llbit = false; // Load-linked bit
  u32 lladr = 0;
  // Interrupt state
  u32 interrupt_mask = 0;
  u32 interrupt_pending = 0;

  // CP0 reference
  CP0 *cp0 = nullptr;
};

/**
 * Abstract CPU interface. All MIPS CPU implementations must inherit from this.
 * The base interpreter implementation has been moved to BaseInterpreter.
 */
class CPU {
public:
  virtual ~CPU() = default;

  // Non-copyable, movable
  CPU(const CPU &) = delete;
  CPU &operator=(const CPU &) = delete;
  CPU(CPU &&) = default;
  CPU &operator=(CPU &&) = default;

  // Initialize CPU state
  virtual void reset(u32 reset_vector = ip32::PROM_RESET_VECTOR) = 0;

  // Execute instructions
  virtual void step() = 0;                   // Execute one instruction
  virtual void run(u64 cycles) = 0;          // Run for N cycles
  virtual void run_until(u32 target_pc) = 0; // Run until PC reaches target

  // Memory access callbacks (for base interpreter compatibility)
  using ReadCallback = std::function<u32(u32 addr, u32 size)>;
  using WriteCallback = std::function<void(u32 addr, u32 size, u32 value)>;

  virtual void set_memory_read_callback(ReadCallback cb) = 0;
  virtual void set_memory_write_callback(WriteCallback cb) = 0;

  // Interrupt handling
  virtual void raise_interrupt(InterruptLine line) = 0;
  virtual void clear_interrupt(InterruptLine line) = 0;

  // State access
  virtual CPUState &state() = 0;
  virtual const CPUState &state() const = 0;

  // CP0 access
  virtual CP0 &cp0() = 0;
  virtual const CP0 &cp0() const = 0;

  // Debugging
  virtual void dump_registers() const = 0;
  virtual void disassemble(u32 addr, char *buffer, size_t size) const = 0;

  // State access (for debugger)
  virtual u32 pc() const = 0;
  virtual u32 cp0_reg(int index) const = 0;

  // Cycle counting
  virtual u64 cycles_executed() const = 0;
  virtual void stop() = 0;
};

} // namespace o2emu::cpu