/**
 * @file mips_r5000.h
 * @brief MIPS R5000 CPU core interface
 */

#pragma once

#include <o2emu/cpu/cpu.h>
#include <o2emu/system/bus.h>

namespace o2emu::cpu {

class MIPSR5000 {
public:
  explicit MIPSR5000(system::Bus *bus);
  ~MIPSR5000();

  // Non-copyable, movable
  MIPSR5000(const MIPSR5000 &) = delete;
  MIPSR5000 &operator=(const MIPSR5000 &) = delete;
  MIPSR5000(MIPSR5000 &&) = default;
  MIPSR5000 &operator=(MIPSR5000 &&) = default;

  // Initialize CPU state
  void reset();

  // Execute for N cycles
  void tick(u64 cycles);

  // State access
  u32 gpr(int reg) const;
  void set_gpr(int reg, u32 value);

  u32 cp0_reg(int reg) const;
  void set_cp0_reg(int reg, u32 value);

  u32 pc() const { return pc_; }
  void set_pc(u32 pc) { pc_ = pc; }

  u64 cycles() const;
  u64 instructions() const;

  // Debugging
  void dump_registers() const;

private:
  system::Bus *bus_;
  CPU::State state_;
  u32 gpr_[32];
  u32 fpr_[32];
  u32 cp0_[32];
  u32 hi_;
  u32 lo_;
  u32 pc_;
  u32 next_pc_;
  bool branch_delay_;
  bool llbit_;
  u32 lladdr_;
  u32 fcr0_;
  u32 fcr31_;
  u64 cycles_;
  u64 instr_count_;

  // CP0 register indices
  static constexpr int CP0_INDEX = 0;
  static constexpr int CP0_RANDOM = 1;
  static constexpr int CP0_ENTRYLO0 = 2;
  static constexpr int CP0_ENTRYLO1 = 3;
  static constexpr int CP0_CONTEXT = 4;
  static constexpr int CP0_PAGEMASK = 5;
  static constexpr int CP0_WIRED = 6;
  static constexpr int CP0_BADVADDR = 8;
  static constexpr int CP0_COUNT = 9;
  static constexpr int CP0_ENTRYHI = 10;
  static constexpr int CP0_COMPARE = 11;
  static constexpr int CP0_STATUS = 12;
  static constexpr int CP0_CAUSE = 13;
  static constexpr int CP0_EPC = 14;
  static constexpr int CP0_PRID = 15;
  static constexpr int CP0_CONFIG = 16;
  static constexpr int CP0_LLADDR = 17;
  static constexpr int CP0_WATCHLO = 18;
  static constexpr int CP0_WATCHHI = 19;
  static constexpr int CP0_XCONTEXT = 20;
  static constexpr int CP0_ECC = 26;
  static constexpr int CP0_CACHEERR = 27;
  static constexpr int CP0_TAGLO = 28;
  static constexpr int CP0_TAGHI = 29;
  static constexpr int CP0_ERROREPC = 30;

  // CP0 Status register bits
  static constexpr u32 STATUS_IE = 0x00000001;
  static constexpr u32 STATUS_EXL = 0x00000002;
  static constexpr u32 STATUS_ERL = 0x00000004;
  static constexpr u32 STATUS_KSU = 0x00000018;
  static constexpr u32 STATUS_UX = 0x00000020;
  static constexpr u32 STATUS_SX = 0x00000040;
  static constexpr u32 STATUS_KX = 0x00000080;
  static constexpr u32 STATUS_IM = 0x0000FF00;
  static constexpr u32 STATUS_BEV = 0x00400000;
  static constexpr u32 STATUS_TS = 0x00800000;
  static constexpr u32 STATUS_SR = 0x01000000;
  static constexpr u32 STATUS_NMI = 0x02000000;

  // CP0 Cause register bits
  static constexpr u32 CAUSE_EXCMASK = 0x0000007C;
  static constexpr u32 CAUSE_IP = 0x0000FF00;
  static constexpr u32 CAUSE_TI = 0x00008000;
  static constexpr u32 CAUSE_BD = 0x80000000;

  // Exception codes
  static constexpr u32 EXC_INT = 0;
  static constexpr u32 EXC_MOD = 1;
  static constexpr u32 EXC_TLBL = 2;
  static constexpr u32 EXC_TLBS = 3;
  static constexpr u32 EXC_ADEL = 4;
  static constexpr u32 EXC_ADES = 5;
  static constexpr u32 EXC_IBE = 6;
  static constexpr u32 EXC_DBE = 7;
  static constexpr u32 EXC_SYS = 8;
  static constexpr u32 EXC_BP = 9;
  static constexpr u32 EXC_RI = 10;
  static constexpr u32 EXC_CPU = 11;
  static constexpr u32 EXC_OV = 12;
  static constexpr u32 EXC_TR = 13;
  static constexpr u32 EXC_FPE = 15;

  // Instruction execution
  void step();
  void execute(u32 instr);
  void execute_special(u32 instr);
  void execute_regimm(u32 instr);
  void execute_j(u32 instr);
  void execute_jal(u32 instr);
  void execute_branch(u32 instr, bool condition);
  void execute_addi(u32 instr);
  void execute_addiu(u32 instr);
  void execute_slti(u32 instr);
  void execute_sltiu(u32 instr);
  void execute_andi(u32 instr);
  void execute_ori(u32 instr);
  void execute_xori(u32 instr);
  void execute_lui(u32 instr);
  void execute_cop0(u32 instr);
  void execute_cop1(u32 instr);
  void execute_load(u32 instr, u32 size, bool sign_extend);
  void execute_store(u32 instr, u32 size);
  void execute_ll(u32 instr);
  void execute_sc(u32 instr);
  void execute_lwc1(u32 instr);
  void execute_swc1(u32 instr);

  // Exception handling
  void exception(u32 exc_code, u32 bad_addr);
  void check_interrupts();
};

} // namespace o2emu::cpu