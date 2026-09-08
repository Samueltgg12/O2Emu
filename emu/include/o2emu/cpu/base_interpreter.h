#pragma once

/**
 * @file base_interpreter.h
 * @brief Base MIPS CPU interpreter implementation (fallback/reference)
 *
 * This is a simple single-issue in-order interpreter for MIPS I/II ISA.
 * It's kept as a reference implementation and fallback. Production code
 * should use MIPSR5000 or MIPSR10000 via the CPU factory.
 */

#include "cpu.h"
#include <functional>

namespace o2emu::cpu {

class BaseInterpreter : public CPU {
public:
  BaseInterpreter();
  ~BaseInterpreter() override;

  // CPU interface
  void reset(u32 reset_vector = ip32::PROM_RESET_VECTOR) override;
  void step() override;
  void run(u64 cycles) override;
  void run_until(u32 target_pc) override;

  void set_memory_read_callback(ReadCallback cb) override;
  void set_memory_write_callback(WriteCallback cb) override;

  void raise_interrupt(InterruptLine line) override;
  void clear_interrupt(InterruptLine line) override;

  CPUState &state() override { return state_; }
  const CPUState &state() const override { return state_; }

  CP0 &cp0() override { return cp0_; }
  const CP0 &cp0() const override { return cp0_; }

  void dump_registers() const override;
  void disassemble(u32 addr, char *buffer, size_t size) const override;

  u32 pc() const override { return state_.pc; }
  u32 cp0_reg(int index) const override {
    return cp0_.read(static_cast<CP0::Register>(index));
  }

  u64 cycles_executed() const override { return cycles_; }
  void stop() override { stop_requested_ = true; }

private:
  CPUState state_;
  ReadCallback mem_read_cb_;
  WriteCallback mem_write_cb_;
  u64 cycles_ = 0;
  bool stop_requested_ = false;
  u32 cur_pc_ = 0; // Address of the current instruction being executed

  CP0 cp0_;

  // Instruction fetch
  u32 fetch32(u32 addr) const;
  u16 fetch16(u32 addr) const;
  u8 fetch8(u32 addr) const;

  // Instruction decode/execute
  void execute(u32 instr);
  void execute_special(u32 instr);
  void execute_regimm(u32 instr);
  void execute_j(u32 instr);
  void execute_jal(u32 instr);
  void execute_branch(u32 instr, bool eq, bool likely);
  void execute_blez(u32 instr, bool likely);
  void execute_bgtz(u32 instr, bool likely);
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
  void execute_ldl(u32 instr);
  void execute_ldr(u32 instr);
  void execute_sdl(u32 instr);
  void execute_sdr(u32 instr);
  void execute_lwl(u32 instr);
  void execute_lwr(u32 instr);
  void execute_swl(u32 instr);
  void execute_swr(u32 instr);
  void execute_ll(u32 instr);
  void execute_sc(u32 instr);
  void execute_lwc1(u32 instr);
  void execute_swc1(u32 instr);

  // Exception handling
  void exception(ExceptionCode code, u32 bad_addr = 0);
  void check_interrupts();

  // Memory access helpers
  u32 read_memory(u32 addr, u32 size);
  void write_memory(u32 addr, u32 size, u32 value);
  u64 read_memory64(u32 addr);
  void write_memory64(u32 addr, u64 value);
};

} // namespace o2emu::cpu