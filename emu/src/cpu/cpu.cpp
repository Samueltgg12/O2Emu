/**
 * @file cpu.cpp
 * @brief MIPS CPU core implementation
 */

#include <cmath>
#include <cstdint>
#include <cstring>
#include <o2emu/cpu/cp0.h>
#include <o2emu/cpu/cpu.h>
#include <o2emu/logging/logger.h>

namespace o2emu::cpu {

CPU::CPU() : mem_read_cb_(nullptr), mem_write_cb_(nullptr), cycles_(0) {
  reset(0);
}

CPU::~CPU() = default;

void CPU::reset(u32 pc) {
  // Manually initialize CPUState since it has non-trivial members (union with
  // cp0 pointer)
  state_.pc = pc;
  state_.delay_slot_pc = 0;
  state_.in_delay_slot = false;
  state_.llbit = false;
  state_.next_pc = pc + 4;
  for (int i = 0; i < 32; ++i) {
    state_.gpr[i] = 0;
  }
  state_.gpr[0] = 0;           // $zero is always 0
  state_.gpr[29] = 0x80000000; // Initial stack pointer (kseg0)
  state_.gpr[28] = 0x80000000; // Global pointer
  state_.hi = 0;
  state_.lo = 0;
  for (int i = 0; i < 32; ++i) {
    state_.fpr[i] = 0;
  }
  state_.fcr0 = 0;
  state_.fcr31 = 0;
  // Preserve cp0 pointer if it exists
  CP0 *cp0_ptr = state_.cp0;
  state_.cp0 = cp0_ptr;

  // Initialize CP0
  if (state_.cp0) {
    state_.cp0->reset();
  }

  // Initialize FPU
  state_.fcr0 = 0;
  state_.fcr31 = 0x00000001; // Default rounding mode
  for (int i = 0; i < 32; ++i) {
    state_.fpr_u[i] = 0;
  }

  cycles_ = 0;

  O2EMU_LOG_INFO_F("CPU reset, PC = 0x%08x", pc);
}

void CPU::step() {
  if (stop_requested_)
    return;

  // Fetch instruction
  u32 instr = fetch32(state_.pc);

  // Execute instruction
  execute(instr);

  // Update CP0 count register
  cp0_.tick(1);

  // Check for interrupts
  check_interrupts();

  cycles_executed_++;

  // $zero is always 0
  state_.gpr[0] = 0;
}

void CPU::run(u64 cycles) {
  for (u64 i = 0; i < cycles && !stop_requested_; ++i) {
    step();
  }
}

void CPU::stop() { stop_requested_ = true; }

u32 CPU::fetch32(u32 addr) {
  if (memory_read_cb_) {
    return memory_read_cb_(addr, 4);
  }
  return 0;
}

u16 CPU::fetch16(u32 addr) {
  if (memory_read_cb_) {
    return memory_read_cb_(addr, 2);
  }
  return 0;
}

u8 CPU::fetch8(u32 addr) {
  if (memory_read_cb_) {
    return memory_read_cb_(addr, 1);
  }
  return 0;
}

void CPU::execute(u32 instr) {
  // Save PC to detect if branch/jump modified it
  u32 pc_before = state_.pc;

  // Decode instruction
  u32 opcode = (instr >> 26) & 0x3F;

  switch (opcode) {
  case 0x00: // SPECIAL
    execute_special(instr);
    break;
  case 0x01: // REGIMM
    execute_regimm(instr);
    break;
  case 0x02: // J
    execute_j(instr);
    break;
  case 0x03: // JAL
    execute_jal(instr);
    break;
  case 0x04: // BEQ
    execute_branch(instr, true);
    break;
  case 0x05: // BNE
    execute_branch(instr, false);
    break;
  case 0x06: // BLEZ
    execute_blez(instr);
    break;
  case 0x07: // BGTZ
    execute_bgtz(instr);
    break;
  case 0x08: // ADDI
    execute_addi(instr);
    break;
  case 0x09: // ADDIU
    execute_addiu(instr);
    break;
  case 0x0A: // SLTI
    execute_slti(instr, true);
    break;
  case 0x0B: // SLTIU
    execute_slti(instr, false);
    break;
  case 0x0C: // ANDI
    execute_andi(instr);
    break;
  case 0x0D: // ORI
    execute_ori(instr);
    break;
  case 0x0E: // XORI
    execute_xori(instr);
    break;
  case 0x0F: // LUI
    execute_lui(instr);
    break;
  case 0x10: // COP0
    execute_cop0(instr);
    break;
  case 0x11: // COP1
    execute_cop1(instr);
    break;
  case 0x12: // COP2 (not implemented)
    exception(Exception::RI);
    break;
  case 0x13: // COP3 (not implemented)
    exception(Exception::RI);
    break;
  case 0x20: // LB
    execute_load(instr, 1, true);
    break;
  case 0x21: // LH
    execute_load(instr, 2, true);
    break;
  case 0x22: // LWL
    execute_lwl(instr);
    break;
  case 0x23: // LW
    execute_load(instr, 4, true);
    break;
  case 0x24: // LBU
    execute_load(instr, 1, false);
    break;
  case 0x25: // LHU
    execute_load(instr, 2, false);
    break;
  case 0x26: // LWR
    execute_lwr(instr);
    break;
  case 0x28: // SB
    execute_store(instr, 1);
    break;
  case 0x29: // SH
    execute_store(instr, 2);
    break;
  case 0x2A: // SWL
    execute_swl(instr);
    break;
  case 0x2B: // SW
    execute_store(instr, 4);
    break;
  case 0x2E: // SWR
    execute_swr(instr);
    break;
  case 0x30: // LL
    execute_ll(instr);
    break;
  case 0x31: // LWC1
    execute_lwc1(instr);
    break;
  case 0x32: // LWC2 (not implemented)
    exception(Exception::RI);
    break;
  case 0x33: // LWC3 (not implemented)
    exception(Exception::RI);
    break;
  case 0x34:                  // LLD
    exception(Exception::RI); // 64-bit only
    break;
  case 0x35:                  // LDC1
    exception(Exception::RI); // 64-bit only
    break;
  case 0x36: // LDC2 (not implemented)
    exception(Exception::RI);
    break;
  case 0x37: // LDC3 (not implemented)
    exception(Exception::RI);
    break;
  case 0x38: // SC
    execute_sc(instr);
    break;
  case 0x39: // SWC1
    execute_swc1(instr);
    break;
  case 0x3A: // SWC2 (not implemented)
    exception(Exception::RI);
    break;
  case 0x3B: // SWC3 (not implemented)
    exception(Exception::RI);
    break;
  case 0x3C:                  // SCD
    exception(Exception::RI); // 64-bit only
    break;
  case 0x3D:                  // SDC1
    exception(Exception::RI); // 64-bit only
    break;
  case 0x3E: // SDC2 (not implemented)
    exception(Exception::RI);
    break;
  case 0x3F: // SDC3 (not implemented)
    exception(Exception::RI);
    break;
  default:
    O2EMU_LOG_WARN_F("Unknown opcode: 0x%08x at PC 0x%08x", opcode, state_.pc);
    exception(Exception::RI);
    break;
  }

  // Advance PC (unless branch/jump modified it)
  if (state_.pc == pc_before) {
    state_.pc += 4;
  }
}

// SPECIAL instructions (opcode 0x00)
void CPU::execute_special(u32 instr) {
  u32 funct = instr & 0x3F;
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  u32 rd = (instr >> 11) & 0x1F;
  u32 shamt = (instr >> 6) & 0x1F;

  switch (funct) {
  case 0x00: // SLL
    state_.gpr[rd] = state_.gpr[rt] << shamt;
    break;
  case 0x02: // SRL
    state_.gpr[rd] = state_.gpr[rt] >> shamt;
    break;
  case 0x03: // SRA
    state_.gpr[rd] = static_cast<i32>(state_.gpr[rt]) >> shamt;
    break;
  case 0x04: // SLLV
    state_.gpr[rd] = state_.gpr[rt] << (state_.gpr[rs] & 0x1F);
    break;
  case 0x06: // SRLV
    state_.gpr[rd] = state_.gpr[rt] >> (state_.gpr[rs] & 0x1F);
    break;
  case 0x07: // SRAV
    state_.gpr[rd] =
        static_cast<i32>(state_.gpr[rt]) >> (state_.gpr[rs] & 0x1F);
    break;
  case 0x08: // JR
    state_.pc = state_.gpr[rs];
    return;  // Don't advance PC
  case 0x09: // JALR
    state_.gpr[rd] = state_.pc + 4;
    state_.pc = state_.gpr[rs];
    return;
  case 0x0C: // SYSCALL
    exception(Exception::SYS);
    break;
  case 0x0D: // BREAK
    exception(Exception::BP);
    break;
  case 0x0F: // SYNC
    // No-op in emulator
    break;
  case 0x10: // MFHI
    state_.gpr[rd] = state_.hi;
    break;
  case 0x11: // MTHI
    state_.hi = state_.gpr[rs];
    break;
  case 0x12: // MFLO
    state_.gpr[rd] = state_.lo;
    break;
  case 0x13: // MTLO
    state_.lo = state_.gpr[rs];
    break;
  case 0x18: // MULT
  {
    i64 a = static_cast<i32>(state_.gpr[rs]);
    i64 b = static_cast<i32>(state_.gpr[rt]);
    i64 result = a * b;
    state_.lo = static_cast<u32>(result);
    state_.hi = static_cast<u32>(result >> 32);
  } break;
  case 0x19: // MULTU
  {
    u64 a = state_.gpr[rs];
    u64 b = state_.gpr[rt];
    u64 result = a * b;
    state_.lo = static_cast<u32>(result);
    state_.hi = static_cast<u32>(result >> 32);
  } break;
  case 0x1A: // DIV
    if (state_.gpr[rt] != 0) {
      i32 a = static_cast<i32>(state_.gpr[rs]);
      i32 b = static_cast<i32>(state_.gpr[rt]);
      state_.lo = static_cast<u32>(a / b);
      state_.hi = static_cast<u32>(a % b);
    }
    break;
  case 0x1B: // DIVU
    if (state_.gpr[rt] != 0) {
      state_.lo = state_.gpr[rs] / state_.gpr[rt];
      state_.hi = state_.gpr[rs] % state_.gpr[rt];
    }
    break;
  case 0x20: // ADD
    state_.gpr[rd] = state_.gpr[rs] + state_.gpr[rt];
    break;
  case 0x21: // ADDU
    state_.gpr[rd] = state_.gpr[rs] + state_.gpr[rt];
    break;
  case 0x22: // SUB
    state_.gpr[rd] = state_.gpr[rs] - state_.gpr[rt];
    break;
  case 0x23: // SUBU
    state_.gpr[rd] = state_.gpr[rs] - state_.gpr[rt];
    break;
  case 0x24: // AND
    state_.gpr[rd] = state_.gpr[rs] & state_.gpr[rt];
    break;
  case 0x25: // OR
    state_.gpr[rd] = state_.gpr[rs] | state_.gpr[rt];
    break;
  case 0x26: // XOR
    state_.gpr[rd] = state_.gpr[rs] ^ state_.gpr[rt];
    break;
  case 0x27: // NOR
    state_.gpr[rd] = ~(state_.gpr[rs] | state_.gpr[rt]);
    break;
  case 0x2A: // SLT
    state_.gpr[rd] =
        (static_cast<i32>(state_.gpr[rs]) < static_cast<i32>(state_.gpr[rt]))
            ? 1
            : 0;
    break;
  case 0x2B: // SLTU
    state_.gpr[rd] = (state_.gpr[rs] < state_.gpr[rt]) ? 1 : 0;
    break;
  case 0x2C: // TGE
    if (static_cast<i32>(state_.gpr[rs]) >= static_cast<i32>(state_.gpr[rt])) {
      exception(Exception::TR);
    }
    break;
  case 0x2D: // TGEU
    if (state_.gpr[rs] >= state_.gpr[rt]) {
      exception(Exception::TR);
    }
    break;
  case 0x2E: // TLT
    if (static_cast<i32>(state_.gpr[rs]) < static_cast<i32>(state_.gpr[rt])) {
      exception(Exception::TR);
    }
    break;
  case 0x2F: // TLTU
    if (state_.gpr[rs] < state_.gpr[rt]) {
      exception(Exception::TR);
    }
    break;
  case 0x30: // TEQ
    if (state_.gpr[rs] == state_.gpr[rt]) {
      exception(Exception::TR);
    }
    break;
  case 0x32: // TNE
    if (state_.gpr[rs] != state_.gpr[rt]) {
      exception(Exception::TR);
    }
    break;
  default:
    O2EMU_LOG_WARN_F("Unknown SPECIAL funct: 0x%02x", funct);
    exception(Exception::RI);
    break;
  }
}

// REGIMM instructions (opcode 0x01)
void CPU::execute_regimm(u32 instr) {
  u32 rt = (instr >> 16) & 0x1F;
  u32 rs = (instr >> 21) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  switch (rt) {
  case 0x00: // BLTZ
    if (static_cast<i32>(state_.gpr[rs]) < 0) {
      state_.pc += 4 + (imm << 2);
      return;
    }
    break;
  case 0x01: // BGEZ
    if (static_cast<i32>(state_.gpr[rs]) >= 0) {
      state_.pc += 4 + (imm << 2);
      return;
    }
    break;
  case 0x10: // BLTZAL
    state_.gpr[31] = state_.pc + 4;
    if (static_cast<i32>(state_.gpr[rs]) < 0) {
      state_.pc += 4 + (imm << 2);
      return;
    }
    break;
  case 0x11: // BGEZAL
    state_.gpr[31] = state_.pc + 4;
    if (static_cast<i32>(state_.gpr[rs]) >= 0) {
      state_.pc += 4 + (imm << 2);
      return;
    }
    break;
  default:
    O2EMU_LOG_WARN_F("Unknown REGIMM rt: 0x%02x", rt);
    exception(Exception::RI);
    break;
  }
}

// J instruction
void CPU::execute_j(u32 instr) {
  u32 target = instr & 0x03FFFFFF;
  state_.pc = (state_.pc & 0xF0000000) | (target << 2);
}

// JAL instruction
void CPU::execute_jal(u32 instr) {
  u32 target = instr & 0x03FFFFFF;
  state_.gpr[31] = state_.pc + 4;
  state_.pc = (state_.pc & 0xF0000000) | (target << 2);
}

// Branch instructions
void CPU::execute_branch(u32 instr, bool eq) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  bool take = eq ? (state_.gpr[rs] == state_.gpr[rt])
                 : (state_.gpr[rs] != state_.gpr[rt]);
  if (take) {
    state_.pc += 4 + (imm << 2);
    return;
  }
}

void CPU::execute_blez(u32 instr) {
  u32 rs = (instr >> 21) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  if (static_cast<i32>(state_.gpr[rs]) <= 0) {
    state_.pc += 4 + (imm << 2);
    return;
  }
}

void CPU::execute_bgtz(u32 instr) {
  u32 rs = (instr >> 21) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  if (static_cast<i32>(state_.gpr[rs]) > 0) {
    state_.pc += 4 + (imm << 2);
    return;
  }
}

// Immediate arithmetic
void CPU::execute_addi(u32 instr) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  state_.gpr[rt] = state_.gpr[rs] + imm;
}

void CPU::execute_addiu(u32 instr) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  state_.gpr[rt] = state_.gpr[rs] + imm;
}

void CPU::execute_slti(u32 instr, bool signed_cmp) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  if (signed_cmp) {
    state_.gpr[rt] = (static_cast<i32>(state_.gpr[rs]) < imm) ? 1 : 0;
  } else {
    state_.gpr[rt] = (state_.gpr[rs] < static_cast<u32>(imm)) ? 1 : 0;
  }
}

void CPU::execute_andi(u32 instr) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  u16 imm = instr & 0xFFFF;

  state_.gpr[rt] = state_.gpr[rs] & imm;
}

void CPU::execute_ori(u32 instr) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  u16 imm = instr & 0xFFFF;

  state_.gpr[rt] = state_.gpr[rs] | imm;
}

void CPU::execute_xori(u32 instr) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  u16 imm = instr & 0xFFFF;

  state_.gpr[rt] = state_.gpr[rs] ^ imm;
}

void CPU::execute_lui(u32 instr) {
  u32 rt = (instr >> 16) & 0x1F;
  u16 imm = instr & 0xFFFF;

  state_.gpr[rt] = imm << 16;
}

// COP0 instructions
void CPU::execute_cop0(u32 instr) {
  u32 fmt = (instr >> 21) & 0x1F;

  switch (fmt) {
  case 0x00: // MFC0
  {
    u32 rt = (instr >> 16) & 0x1F;
    u32 rd = (instr >> 11) & 0x1F;
    state_.gpr[rt] = cp0_.read(static_cast<CP0::Register>(rd));
  } break;
  case 0x04: // MTC0
  {
    u32 rt = (instr >> 16) & 0x1F;
    u32 rd = (instr >> 11) & 0x1F;
    cp0_.write(static_cast<CP0::Register>(rd), state_.gpr[rt]);
  } break;
  case 0x10: // COP0 ops
  {
    u32 funct = instr & 0x3F;
    switch (funct) {
    case 0x01: // TLBR
      // Not implemented
      break;
    case 0x02: // TLBWI
      // Not implemented
      break;
    case 0x06: // TLBWR
      // Not implemented
      break;
    case 0x08: // TLBP
      // Not implemented
      break;
    case 0x10: // RFE
      cp0_.rfe();
      break;
    case 0x18: // ERET
      cp0_.eret(state_);
      return; // PC modified
    default:
      O2EMU_LOG_WARN_F("Unknown COP0 funct: 0x%02x", funct);
      exception(Exception::RI);
      break;
    }
  } break;
  default:
    O2EMU_LOG_WARN_F("Unknown COP0 fmt: 0x%02x", fmt);
    exception(Exception::RI);
    break;
  }
}

// COP1 (FPU) instructions
void CPU::execute_cop1(u32 instr) {
  u32 fmt = (instr >> 21) & 0x1F;

  switch (fmt) {
  case 0x00: // MFC1
  {
    u32 rt = (instr >> 16) & 0x1F;
    u32 fs = (instr >> 11) & 0x1F;
    state_.gpr[rt] = state_.fpr_u[fs];
  } break;
  case 0x02: // CFC1
  {
    u32 rt = (instr >> 16) & 0x1F;
    u32 fs = (instr >> 11) & 0x1F;
    if (fs == 0)
      state_.gpr[rt] = state_.fcr0;
    else if (fs == 31)
      state_.gpr[rt] = state_.fcr31;
  } break;
  case 0x04: // MTC1
  {
    u32 rt = (instr >> 16) & 0x1F;
    u32 fs = (instr >> 11) & 0x1F;
    state_.fpr_u[fs] = state_.gpr[rt];
  } break;
  case 0x06: // CTC1
  {
    u32 rt = (instr >> 16) & 0x1F;
    u32 fs = (instr >> 11) & 0x1F;
    if (fs == 0)
      state_.fcr0 = state_.gpr[rt];
    else if (fs == 31)
      state_.fcr31 = state_.gpr[rt];
  } break;
  case 0x10: // FPU arithmetic
    execute_fpu_arith(instr);
    break;
  default:
    O2EMU_LOG_WARN_F("Unknown COP1 fmt: 0x%02x", fmt);
    exception(Exception::RI);
    break;
  }
}

// FPU arithmetic
void CPU::execute_fpu_arith(u32 instr) {
  u32 ft = (instr >> 16) & 0x1F;
  u32 fs = (instr >> 11) & 0x1F;
  u32 fd = (instr >> 6) & 0x1F;
  u32 funct = instr & 0x3F;

  // For now, just handle basic single-precision ops
  float fs_val = state_.fpr_s[fs];
  float ft_val = state_.fpr_s[ft];
  float result = 0.0f;

  switch (funct) {
  case 0x00: // ADD.S
    result = fs_val + ft_val;
    break;
  case 0x01: // SUB.S
    result = fs_val - ft_val;
    break;
  case 0x02: // MUL.S
    result = fs_val * ft_val;
    break;
  case 0x03: // DIV.S
    result = fs_val / ft_val;
    break;
  case 0x04: // SQRT.S
    result = sqrtf(fs_val);
    break;
  case 0x05: // ABS.S
    result = fabsf(fs_val);
    break;
  case 0x06: // MOV.S
    result = fs_val;
    break;
  case 0x07: // NEG.S
    result = -fs_val;
    break;
  case 0x08: // ROUND.L.S
  case 0x09: // TRUNC.L.S
  case 0x0A: // CEIL.L.S
  case 0x0B: // FLOOR.L.S
  case 0x0C: // ROUND.W.S
  case 0x0D: // TRUNC.W.S
  case 0x0E: // CEIL.W.S
  case 0x0F: // FLOOR.W.S
    // Conversion ops - not fully implemented
    break;
  case 0x20: // CVT.S.W
    result = static_cast<float>(static_cast<i32>(state_.fpr_u[fs]));
    break;
  case 0x24: // CVT.W.S
    state_.fpr_u[fd] = static_cast<u32>(static_cast<i32>(fs_val));
    return;
  case 0x30: // C.F.S
  case 0x31: // C.UN.S
  case 0x32: // C.EQ.S
  case 0x33: // C.UEQ.S
  case 0x34: // C.OLT.S
  case 0x35: // C.ULT.S
  case 0x36: // C.OLE.S
  case 0x37: // C.ULE.S
  case 0x38: // C.SF.S
  case 0x39: // C.NGLE.S
  case 0x3A: // C.SEQ.S
  case 0x3B: // C.NGL.S
  case 0x3C: // C.LT.S
  case 0x3D: // C.NGE.S
  case 0x3E: // C.LE.S
  case 0x3F: // C.NGT.S
    // Comparison ops - set condition bit in FCR31
    break;
  default:
    O2EMU_LOG_WARN_F("Unknown FPU funct: 0x%02x", funct);
    exception(Exception::RI);
    return;
  }

  state_.fpr_s[fd] = result;
}

// Load instructions
void CPU::execute_load(u32 instr, u32 size, bool sign_extend) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  u32 addr = state_.gpr[rs] + imm;
  u32 value = 0;

  if (memory_read_cb_) {
    switch (size) {
    case 1:
      value = memory_read_cb_(addr, 1);
      break;
    case 2:
      value = memory_read_cb_(addr, 2);
      break;
    case 4:
      value = memory_read_cb_(addr, 4);
      break;
    }
  }

  if (sign_extend) {
    switch (size) {
    case 1:
      state_.gpr[rt] = static_cast<i32>(static_cast<i8>(value));
      break;
    case 2:
      state_.gpr[rt] = static_cast<i32>(static_cast<i16>(value));
      break;
    case 4:
      state_.gpr[rt] = value;
      break;
    }
  } else {
    state_.gpr[rt] = value;
  }
}

void CPU::execute_lwl(u32 instr) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  u32 addr = state_.gpr[rs] + imm;
  u32 aligned_addr = addr & ~3;
  u32 word = memory_read_cb_ ? memory_read_cb_(aligned_addr, 4) : 0;

  int shift = (addr & 3) * 8;
  u32 mask = 0xFFFFFFFF << (32 - shift);
  state_.gpr[rt] = (state_.gpr[rt] & ~mask) | (word & mask);
}

void CPU::execute_lwr(u32 instr) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  u32 addr = state_.gpr[rs] + imm;
  u32 aligned_addr = addr & ~3;
  u32 word = memory_read_cb_ ? memory_read_cb_(aligned_addr, 4) : 0;

  int shift = (3 - (addr & 3)) * 8;
  u32 mask = 0xFFFFFFFF >> (32 - shift);
  state_.gpr[rt] = (state_.gpr[rt] & ~mask) | (word & mask);
}

// Store instructions
void CPU::execute_store(u32 instr, u32 size) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  u32 addr = state_.gpr[rs] + imm;
  u32 value = state_.gpr[rt];

  if (memory_write_cb_) {
    memory_write_cb_(addr, size, value);
  }
}

void CPU::execute_swl(u32 instr) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  u32 addr = state_.gpr[rs] + imm;
  u32 aligned_addr = addr & ~3;
  u32 word = memory_read_cb_ ? memory_read_cb_(aligned_addr, 4) : 0;

  int shift = (addr & 3) * 8;
  u32 mask = 0xFFFFFFFF << (32 - shift);
  word = (word & ~mask) | (state_.gpr[rt] & mask);

  if (memory_write_cb_) {
    memory_write_cb_(aligned_addr, 4, word);
  }
}

void CPU::execute_swr(u32 instr) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  u32 addr = state_.gpr[rs] + imm;
  u32 aligned_addr = addr & ~3;
  u32 word = memory_read_cb_ ? memory_read_cb_(aligned_addr, 4) : 0;

  int shift = (3 - (addr & 3)) * 8;
  u32 mask = 0xFFFFFFFF >> (32 - shift);
  word = (word & ~mask) | (state_.gpr[rt] & mask);

  if (memory_write_cb_) {
    memory_write_cb_(aligned_addr, 4, word);
  }
}

// LL/SC
void CPU::execute_ll(u32 instr) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  u32 addr = state_.gpr[rs] + imm;
  state_.gpr[rt] = memory_read_cb_ ? memory_read_cb_(addr, 4) : 0;
  // TODO: Set LL bit for SC
}

void CPU::execute_sc(u32 instr) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  u32 addr = state_.gpr[rs] + imm;
  // TODO: Check LL bit
  if (memory_write_cb_) {
    memory_write_cb_(addr, 4, state_.gpr[rt]);
  }
  state_.gpr[rt] = 1; // Success
}

// FPU loads/stores
void CPU::execute_lwc1(u32 instr) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 ft = (instr >> 16) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  u32 addr = state_.gpr[rs] + imm;
  state_.fpr_u[ft] = memory_read_cb_ ? memory_read_cb_(addr, 4) : 0;
}

void CPU::execute_swc1(u32 instr) {
  u32 rs = (instr >> 21) & 0x1F;
  u32 ft = (instr >> 16) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  u32 addr = state_.gpr[rs] + imm;
  if (memory_write_cb_) {
    memory_write_cb_(addr, 4, state_.fpr_u[ft]);
  }
}

// Exception handling
void CPU::exception(Exception exc) {
  cp0_.exception(exc, state_);
  state_.pc = cp0_.epc(); // Exception handler address
}

void CPU::check_interrupts() {
  u32 pending = cp0_.pending_interrupts();
  if (pending && (cp0_.status() & 0x0001) &&
      (cp0_.status() & 0x0004)) { // IE and IM bits
    exception(Exception::INT);
  }
}

void CPU::disassemble(u32 addr, char *buffer, size_t size) const {
  u32 instr = fetch32(addr);
  u32 opcode = (instr >> 26) & 0x3F;

  std::snprintf(buffer, size, "0x%08X: ", addr);
  char *p = buffer + std::strlen(buffer);
  size_t remaining = size - std::strlen(buffer);

  // Simple disassembly - just show opcode for now
  std::snprintf(p, remaining, "0x%08X  ", instr);
}

void CPU::dump_registers() const {
  O2EMU_LOG_INFO("=== CPU Registers ===");
  O2EMU_LOG_INFO_F("PC: 0x%08x", state_.pc);
  for (int i = 0; i < 32; ++i) {
    O2EMU_LOG_INFO_F("$%d: 0x%08x", i, state_.gpr[i]);
  }
  O2EMU_LOG_INFO_F("HI: 0x%08x LO: 0x%08x", state_.hi, state_.lo);
  cp0_.dump();
}

} // namespace o2emu::cpu