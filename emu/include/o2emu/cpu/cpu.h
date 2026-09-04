#pragma once

/**
 * @file cpu.h
 * @brief MIPS CPU core interface
 */

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
  u32 gpr[32] = {}; // $0-$31

  // Special registers
  u32 pc = 0; // Program counter
  u32 hi = 0; // Multiply/divide high
  u32 lo = 0; // Multiply/divide low

  // FPU registers (32 single-precision or 16 double-precision)
  union {
    float fpr_s[32];
    double fpr_d[16];
    u32 fpr_u[32];
  };
  u32 fcr0 = 0;  // FPU control/status
  u32 fcr31 = 0; // FPU control/status register

  // Execution state
  bool in_delay_slot = false;
  u32 delay_slot_pc = 0;
  bool llbit = false; // Load-linked bit

  // Interrupt state
  u32 interrupt_mask = 0;
  u32 interrupt_pending = 0;
};

class CPU {
public:
  CPU();
  ~CPU();

  // Non-copyable, movable
  CPU(const CPU &) = delete;
  CPU &operator=(const CPU &) = delete;
  CPU(CPU &&) = default;
  CPU &operator=(CPU &&) = default;

  // Initialize CPU state
  void reset(u32 reset_vector = ip32::PROM_RESET_VECTOR);

  // Execute instructions
  void step();                   // Execute one instruction
  void run(u64 cycles);          // Run for N cycles
  void run_until(u32 target_pc); // Run until PC reaches target

  // Memory access callbacks
  using ReadCallback = std::function<u32(u32 addr, u32 size)>;
  using WriteCallback = std::function<void(u32 addr, u32 size, u32 value)>;

  void set_memory_read_callback(ReadCallback cb) {
    memory_read_cb_ = std::move(cb);
  }
  void set_memory_write_callback(WriteCallback cb) {
    memory_write_cb_ = std::move(cb);
  }

  // Interrupt handling
  void raise_interrupt(InterruptLine line);
  void clear_interrupt(InterruptLine line);

  // State access
  CPUState &state() { return state_; }
  const CPUState &state() const { return state_; }

  // CP0 access
  CP0 &cp0() { return cp0_; }
  const CP0 &cp0() const { return cp0_; }

  // Debugging
  void dump_registers() const;
  void disassemble(u32 addr, char *buffer, size_t size) const;

  // Cycle counting
  u64 cycles_executed() const { return cycles_executed_; }

private:
  CPUState state_;
  u64 cycles_executed_ = 0;
  bool stop_requested_ = false;

  ReadCallback memory_read_cb_;
  WriteCallback memory_write_cb_;

  CP0 cp0_;

  // Instruction fetch
  u32 fetch32(u32 addr);
  u16 fetch16(u32 addr);
  u8 fetch8(u32 addr);

  // Instruction decode/execute
  void execute(u32 instr);
  void execute_special(u32 instr);
  void execute_regimm(u32 instr);
  void execute_j(u32 instr);
  void execute_jal(u32 instr);
  void execute_branch(u32 instr);
  void execute_blez(u32 instr);
  void execute_bgtz(u32 instr);
  void execute_addi(u32 instr);
  void execute_addiu(u32 instr);
  void execute_slti(u32 instr, bool signed_cmp);
  void execute_andi(u32 instr);
  void execute_ori(u32 instr);
  void execute_xori(u32 instr);
  void execute_lui(u32 instr);
  void execute_cop0(u32 instr);
  void execute_cop1(u32 instr);
  void execute_fpu_arith(u32 instr);

  // Load/store instructions
  void execute_load(u32 instr, u32 size, bool sign_extend);
  void execute_store(u32 instr, u32 size);
  void execute_lwl(u32 instr);
  void execute_lwr(u32 instr);
  void execute_swl(u32 instr);
  void execute_swr(u32 instr);
  void execute_ll(u32 instr);
  void execute_sc(u32 instr);
  void execute_lwc1(u32 instr);
  void execute_swc1(u32 instr);

  // Exception handling
  void exception(ExceptionCode code);
  void check_interrupts();

  // Memory access helpers
  u32 read_memory(u32 addr, u32 size);
  void write_memory(u32 addr, u32 size, u32 value);

  // Exception handling (legacy names)
  void handle_exception(ExceptionCode code, u32 bad_addr = 0);
  void return_from_exception();
};

} // namespace o2emu::cpu