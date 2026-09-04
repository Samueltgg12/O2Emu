/**
 * @file mips_r5000.cpp
 * @brief MIPS R5000 CPU core implementation
 */

#include <algorithm>
#include <cstring>
#include <o2emu/cpu/mips_r5000.h>
#include <o2emu/logging/logger.h>

namespace o2emu::cpu {

MIPSR5000::MIPSR5000(system::Bus *bus)
    : bus_(bus), hi_(0), lo_(0), pc_(0), next_pc_(0), branch_delay_(false),
      llbit_(false), cycles_(0), instr_count_(0) {
  reset();
}

MIPSR5000::~MIPSR5000() = default;

void MIPSR5000::reset() {
  std::memset(gpr_, 0, sizeof(gpr_));
  std::memset(fpr_, 0, sizeof(fpr_));
  std::memset(cp0_, 0, sizeof(cp0_));

  hi_ = 0;
  lo_ = 0;
  pc_ = 0xBFC00000; // Reset vector
  next_pc_ = pc_ + 4;
  branch_delay_ = false;
  llbit_ = false;
  cycles_ = 0;
  instr_count_ = 0;

  // Initialize CP0 registers
  cp0_[CP0_INDEX] = 0;
  cp0_[CP0_RANDOM] = 31;
  cp0_[CP0_ENTRYLO0] = 0;
  cp0_[CP0_ENTRYLO1] = 0;
  cp0_[CP0_CONTEXT] = 0;
  cp0_[CP0_PAGEMASK] = 0;
  cp0_[CP0_WIRED] = 0;
  cp0_[CP0_BADVADDR] = 0;
  cp0_[CP0_COUNT] = 0;
  cp0_[CP0_ENTRYHI] = 0;
  cp0_[CP0_COMPARE] = 0;
  cp0_[CP0_STATUS] = 0x00400004; // BEV=1, TS=1, SR=0, NMI=0
  cp0_[CP0_CAUSE] = 0;
  cp0_[CP0_EPC] = 0;
  cp0_[CP0_PRID] = 0x00002000; // R5000
  cp0_[CP0_CONFIG] = 0x00000000;
  cp0_[CP0_LLADDR] = 0;
  cp0_[CP0_WATCHLO] = 0;
  cp0_[CP0_WATCHHI] = 0;
  cp0_[CP0_XCONTEXT] = 0;
  cp0_[CP0_ECC] = 0;
  cp0_[CP0_CACHEERR] = 0;
  cp0_[CP0_TAGLO] = 0;
  cp0_[CP0_TAGHI] = 0;
  cp0_[CP0_ERROREPC] = 0;

  // Initialize FPR
  for (int i = 0; i < 32; ++i) {
    fpr_[i] = 0;
  }
  fcr0_ = 0;
  fcr31_ = 0x00000000; // Round to nearest, all exceptions disabled
}

void MIPSR5000::tick(u64 cycles) {
  (void)cycles; // Suppress unused parameter warning
  cycles_ += cycles;

  // Execute instructions
  for (u64 i = 0; i < cycles; ++i) {
    step();
  }

  // Update CP0 Count register
  cp0_[CP0_COUNT] =
      static_cast<u32>(cycles_ / 2); // Count increments every 2 cycles

  // Check for timer interrupt
  if (cp0_[CP0_COUNT] >= cp0_[CP0_COMPARE]) {
    cp0_[CP0_CAUSE] |= CAUSE_TI; // Timer interrupt
  }
}

void MIPSR5000::step() {
  if (branch_delay_) {
    pc_ = next_pc_;
    branch_delay_ = false;
  } else {
    pc_ = next_pc_;
    next_pc_ = pc_ + 4;
  }

  // Fetch instruction
  u32 instr = bus_->read32(pc_);
  // Note: Bus read32 doesn't return success/failure, it just returns the value
  // If we need error handling, we'd need to add exception support to Bus

  // Decode and execute
  execute(instr);

  instr_count_++;

  // Check for interrupts
  check_interrupts();
}

void MIPSR5000::execute(u32 instr) {
  u32 opcode = instr >> 26;

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
    execute_branch(instr,
                   gpr_[(instr >> 21) & 0x1F] == gpr_[(instr >> 16) & 0x1F]);
    break;
  case 0x05: // BNE
    execute_branch(instr,
                   gpr_[(instr >> 21) & 0x1F] != gpr_[(instr >> 16) & 0x1F]);
    break;
  case 0x06: // BLEZ
    execute_branch(instr, static_cast<i32>(gpr_[(instr >> 21) & 0x1F]) <= 0);
    break;
  case 0x07: // BGTZ
    execute_branch(instr, static_cast<i32>(gpr_[(instr >> 21) & 0x1F]) > 0);
    break;
  case 0x08: // ADDI
    execute_addi(instr);
    break;
  case 0x09: // ADDIU
    execute_addiu(instr);
    break;
  case 0x0A: // SLTI
    execute_slti(instr);
    break;
  case 0x0B: // SLTIU
    execute_sltiu(instr);
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
  case 0x12: // COP2
    // Not implemented
    exception(EXC_RI, pc_);
    break;
  case 0x13: // COP3
    // Not implemented
    exception(EXC_RI, pc_);
    break;
  case 0x20: // LB
    execute_load(instr, 1, true);
    break;
  case 0x21: // LH
    execute_load(instr, 2, true);
    break;
  case 0x22: // LWL
    // Not implemented
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
    // Not implemented
    break;
  case 0x28: // SB
    execute_store(instr, 1);
    break;
  case 0x29: // SH
    execute_store(instr, 2);
    break;
  case 0x2A: // SWL
    // Not implemented
    break;
  case 0x2B: // SW
    execute_store(instr, 4);
    break;
  case 0x2C: // SWR
    // Not implemented
    break;
  case 0x2D: // CACHE
    // Not implemented
    break;
  case 0x2F: // PREF
    // Not implemented
    break;
  case 0x30: // LL
    execute_ll(instr);
    break;
  case 0x31: // LWC1
    execute_lwc1(instr);
    break;
  case 0x32: // LWC2
    // Not implemented
    break;
  case 0x33: // PREF
    // Not implemented
    break;
  case 0x38: // SC
    execute_sc(instr);
    break;
  case 0x39: // SWC1
    execute_swc1(instr);
    break;
  case 0x3A: // SWC2
    // Not implemented
    break;
  default:
    O2EMU_LOG_WARN_F("Unknown opcode: 0x%x at PC=0x%08x", opcode, pc_);
    exception(EXC_RI, pc_);
    break;
  }
}

void MIPSR5000::execute_special(u32 instr) {
  u32 funct = instr & 0x3F;
  u32 rs = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  u32 rd = (instr >> 11) & 0x1F;
  u32 shamt = (instr >> 6) & 0x1F;

  switch (funct) {
  case 0x00: // SLL
    if (rd != 0)
      gpr_[rd] = gpr_[rt] << shamt;
    break;
  case 0x02: // SRL
    if (rd != 0)
      gpr_[rd] = gpr_[rt] >> shamt;
    break;
  case 0x03: // SRA
    if (rd != 0)
      gpr_[rd] = static_cast<i32>(gpr_[rt]) >> shamt;
    break;
  case 0x04: // SLLV
    if (rd != 0)
      gpr_[rd] = gpr_[rt] << (gpr_[rs] & 0x1F);
    break;
  case 0x06: // SRLV
    if (rd != 0)
      gpr_[rd] = gpr_[rt] >> (gpr_[rs] & 0x1F);
    break;
  case 0x07: // SRAV
    if (rd != 0)
      gpr_[rd] = static_cast<i32>(gpr_[rt]) >> (gpr_[rs] & 0x1F);
    break;
  case 0x08: // JR
    next_pc_ = gpr_[rs];
    branch_delay_ = true;
    break;
  case 0x09: // JALR
    if (rd != 0)
      gpr_[rd] = pc_ + 8;
    next_pc_ = gpr_[rs];
    branch_delay_ = true;
    break;
  case 0x0C: // SYSCALL
    exception(EXC_SYS, pc_);
    break;
  case 0x0D: // BREAK
    exception(EXC_BP, pc_);
    break;
  case 0x10: // MFHI
    if (rd != 0)
      gpr_[rd] = hi_;
    break;
  case 0x11: // MTHI
    hi_ = gpr_[rs];
    break;
  case 0x12: // MFLO
    if (rd != 0)
      gpr_[rd] = lo_;
    break;
  case 0x13: // MTLO
    lo_ = gpr_[rs];
    break;
  case 0x18: // MULT
  {
    i64 result = static_cast<i64>(static_cast<i32>(gpr_[rs])) *
                 static_cast<i64>(static_cast<i32>(gpr_[rt]));
    lo_ = static_cast<u32>(result);
    hi_ = static_cast<u32>(result >> 32);
  } break;
  case 0x19: // MULTU
  {
    u64 result = static_cast<u64>(gpr_[rs]) * static_cast<u64>(gpr_[rt]);
    lo_ = static_cast<u32>(result);
    hi_ = static_cast<u32>(result >> 32);
  } break;
  case 0x1A: // DIV
    if (gpr_[rt] != 0) {
      lo_ = static_cast<u32>(static_cast<i32>(gpr_[rs]) /
                             static_cast<i32>(gpr_[rt]));
      hi_ = static_cast<u32>(static_cast<i32>(gpr_[rs]) %
                             static_cast<i32>(gpr_[rt]));
    }
    break;
  case 0x1B: // DIVU
    if (gpr_[rt] != 0) {
      lo_ = gpr_[rs] / gpr_[rt];
      hi_ = gpr_[rs] % gpr_[rt];
    }
    break;
  case 0x20: // ADD
    if (rd != 0)
      gpr_[rd] = gpr_[rs] + gpr_[rt];
    break;
  case 0x21: // ADDU
    if (rd != 0)
      gpr_[rd] = gpr_[rs] + gpr_[rt];
    break;
  case 0x22: // SUB
    if (rd != 0)
      gpr_[rd] = gpr_[rs] - gpr_[rt];
    break;
  case 0x23: // SUBU
    if (rd != 0)
      gpr_[rd] = gpr_[rs] - gpr_[rt];
    break;
  case 0x24: // AND
    if (rd != 0)
      gpr_[rd] = gpr_[rs] & gpr_[rt];
    break;
  case 0x25: // OR
    if (rd != 0)
      gpr_[rd] = gpr_[rs] | gpr_[rt];
    break;
  case 0x26: // XOR
    if (rd != 0)
      gpr_[rd] = gpr_[rs] ^ gpr_[rt];
    break;
  case 0x27: // NOR
    if (rd != 0)
      gpr_[rd] = ~(gpr_[rs] | gpr_[rt]);
    break;
  case 0x2A: // SLT
    if (rd != 0)
      gpr_[rd] =
          (static_cast<i32>(gpr_[rs]) < static_cast<i32>(gpr_[rt])) ? 1 : 0;
    break;
  case 0x2B: // SLTU
    if (rd != 0)
      gpr_[rd] = (gpr_[rs] < gpr_[rt]) ? 1 : 0;
    break;
  default:
    O2EMU_LOG_WARN_F("Unknown SPECIAL funct: 0x%x", funct);
    exception(EXC_RI, pc_);
    break;
  }
}

void MIPSR5000::execute_regimm(u32 instr) {
  u32 rt = (instr >> 16) & 0x1F;
  u32 rs = (instr >> 21) & 0x1F;
  // i16 imm = static_cast<i16>(instr & 0xFFFF); // Unused for now

  switch (rt) {
  case 0x00: // BLTZ
    execute_branch(instr, static_cast<i32>(gpr_[rs]) < 0);
    break;
  case 0x01: // BGEZ
    execute_branch(instr, static_cast<i32>(gpr_[rs]) >= 0);
    break;
  case 0x10: // BLTZAL
    if (31 != 0)
      gpr_[31] = pc_ + 8;
    execute_branch(instr, static_cast<i32>(gpr_[rs]) < 0);
    break;
  case 0x11: // BGEZAL
    if (31 != 0)
      gpr_[31] = pc_ + 8;
    execute_branch(instr, static_cast<i32>(gpr_[rs]) >= 0);
    break;
  default:
    O2EMU_LOG_WARN_F("Unknown REGIMM rt: 0x%x", rt);
    exception(EXC_RI, pc_);
    break;
  }
}

void MIPSR5000::execute_j(u32 instr) {
  u32 target = (instr & 0x03FFFFFF) << 2;
  next_pc_ = (pc_ & 0xF0000000) | target;
  branch_delay_ = true;
}

void MIPSR5000::execute_jal(u32 instr) {
  u32 target = (instr & 0x03FFFFFF) << 2;
  gpr_[31] = pc_ + 8;
  next_pc_ = (pc_ & 0xF0000000) | target;
  branch_delay_ = true;
}

void MIPSR5000::execute_branch(u32 instr, bool condition) {
  i16 offset = static_cast<i16>(instr & 0xFFFF);
  if (condition) {
    next_pc_ = pc_ + 4 + (offset << 2);
    branch_delay_ = true;
  }
}

void MIPSR5000::execute_addi(u32 instr) {
  u32 rt = (instr >> 16) & 0x1F;
  u32 rs = (instr >> 21) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  if (rt != 0) {
    i32 result = static_cast<i32>(gpr_[rs]) + imm;
    gpr_[rt] = static_cast<u32>(result);
  }
}

void MIPSR5000::execute_addiu(u32 instr) {
  u32 rt = (instr >> 16) & 0x1F;
  u32 rs = (instr >> 21) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  if (rt != 0) {
    gpr_[rt] = gpr_[rs] + static_cast<u32>(imm);
  }
}

void MIPSR5000::execute_slti(u32 instr) {
  u32 rt = (instr >> 16) & 0x1F;
  u32 rs = (instr >> 21) & 0x1F;
  i16 imm = static_cast<i16>(instr & 0xFFFF);

  if (rt != 0) {
    gpr_[rt] = (static_cast<i32>(gpr_[rs]) < imm) ? 1 : 0;
  }
}

void MIPSR5000::execute_sltiu(u32 instr) {
  u32 rt = (instr >> 16) & 0x1F;
  u32 rs = (instr >> 21) & 0x1F;
  u16 imm = static_cast<u16>(instr & 0xFFFF);

  if (rt != 0) {
    gpr_[rt] = (gpr_[rs] < imm) ? 1 : 0;
  }
}

void MIPSR5000::execute_andi(u32 instr) {
  u32 rt = (instr >> 16) & 0x1F;
  u32 rs = (instr >> 21) & 0x1F;
  u16 imm = static_cast<u16>(instr & 0xFFFF);

  if (rt != 0) {
    gpr_[rt] = gpr_[rs] & imm;
  }
}

void MIPSR5000::execute_ori(u32 instr) {
  u32 rt = (instr >> 16) & 0x1F;
  u32 rs = (instr >> 21) & 0x1F;
  u16 imm = static_cast<u16>(instr & 0xFFFF);

  if (rt != 0) {
    gpr_[rt] = gpr_[rs] | imm;
  }
}

void MIPSR5000::execute_xori(u32 instr) {
  u32 rt = (instr >> 16) & 0x1F;
  u32 rs = (instr >> 21) & 0x1F;
  u16 imm = static_cast<u16>(instr & 0xFFFF);

  if (rt != 0) {
    gpr_[rt] = gpr_[rs] ^ imm;
  }
}

void MIPSR5000::execute_lui(u32 instr) {
  u32 rt = (instr >> 16) & 0x1F;
  u16 imm = static_cast<u16>(instr & 0xFFFF);

  if (rt != 0) {
    gpr_[rt] = imm << 16;
  }
}

void MIPSR5000::execute_cop0(u32 instr) {
  u32 fmt = (instr >> 21) & 0x1F;
  u32 rt = (instr >> 16) & 0x1F;
  u32 rd = (instr >> 11) & 0x1F;

  switch (fmt) {
  case 0x00: // MFC0
    if (rt != 0)
      gpr_[rt] = cp0_[rd];
    break;
  case 0x04: // MTC0
    cp0_[rd] = gpr_[rt];
    break;
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
    case 0x10: // ERET
      pc_ = cp0_[CP0_EPC];
      next_pc_ = pc_ + 4;
      cp0_[CP0_STATUS] &= ~STATUS_ERL;
      break;
    case 0x18: // WAIT
      // Not implemented
      break;
    default:
      O2EMU_LOG_WARN_F("Unknown COP0 funct: 0x%x", funct);
      exception(EXC_RI, pc_);
      break;
    }
  } break;
  default:
    O2EMU_LOG_WARN_F("Unknown COP0 fmt: 0x%x", fmt);
    exception(EXC_RI, pc_);
    break;
  }
}

void MIPSR5000::execute_cop1(u32 instr) {
  // FPU instructions - simplified implementation
  u32 fmt = (instr >> 21) & 0x1F;

  switch (fmt) {
  case 0x00: // MFC1
  {
    u32 rt = (instr >> 16) & 0x1F;
    u32 fs = (instr >> 11) & 0x1F;
    if (rt != 0)
      gpr_[rt] = fpr_[fs];
  } break;
  case 0x04: // MTC1
  {
    u32 rt = (instr >> 16) & 0x1F;
    u32 fs = (instr >> 11) & 0x1F;
    fpr_[fs] = gpr_[rt];
  } break;
  case 0x10: // COP1 ops
    // FPU arithmetic - not fully implemented
    break;
  default:
    O2EMU_LOG_WARN_F("Unknown COP1 fmt: 0x%x", fmt);
    exception(EXC_RI, pc_);
    break;
  }
}

void MIPSR5000::execute_load(u32 instr, u32 size, bool sign_extend) {
  u32 rt = (instr >> 16) & 0x1F;
  u32 rs = (instr >> 21) & 0x1F;
  i16 offset = static_cast<i16>(instr & 0xFFFF);

  u32 addr = gpr_[rs] + static_cast<u32>(offset);
  u32 value = 0;

  switch (size) {
  case 1:
    value = bus_->read8(addr);
    break;
  case 2:
    value = bus_->read16(addr);
    break;
  case 4:
    value = bus_->read32(addr);
    break;
  }

  if (rt != 0) {
    if (sign_extend) {
      switch (size) {
      case 1:
        gpr_[rt] = static_cast<u32>(static_cast<i8>(value));
        break;
      case 2:
        gpr_[rt] = static_cast<u32>(static_cast<i16>(value));
        break;
      case 4:
        gpr_[rt] = value;
        break;
      }
    } else {
      gpr_[rt] = value;
    }
  }
}

void MIPSR5000::execute_store(u32 instr, u32 size) {
  u32 rt = (instr >> 16) & 0x1F;
  u32 rs = (instr >> 21) & 0x1F;
  i16 offset = static_cast<i16>(instr & 0xFFFF);

  u32 addr = gpr_[rs] + static_cast<u32>(offset);
  u32 value = gpr_[rt];

  switch (size) {
  case 1:
    bus_->write8(addr, value);
    break;
  case 2:
    bus_->write16(addr, value);
    break;
  case 4:
    bus_->write32(addr, value);
    break;
  }
}

void MIPSR5000::execute_ll(u32 instr) {
  u32 rt = (instr >> 16) & 0x1F;
  u32 rs = (instr >> 21) & 0x1F;
  i16 offset = static_cast<i16>(instr & 0xFFFF);

  u32 addr = gpr_[rs] + static_cast<u32>(offset);
  u32 value = bus_->read32(addr);

  if (rt != 0)
    gpr_[rt] = value;
  llbit_ = true;
  lladdr_ = addr;
}

void MIPSR5000::execute_sc(u32 instr) {
  u32 rt = (instr >> 16) & 0x1F;
  u32 rs = (instr >> 21) & 0x1F;
  i16 offset = static_cast<i16>(instr & 0xFFFF);

  u32 addr = gpr_[rs] + static_cast<u32>(offset);

  if (llbit_ && addr == lladdr_) {
    bus_->write32(addr, gpr_[rt]);
    if (rt != 0)
      gpr_[rt] = 1;
  } else {
    if (rt != 0)
      gpr_[rt] = 0;
  }
  llbit_ = false;
}

void MIPSR5000::execute_lwc1(u32 instr) {
  u32 ft = (instr >> 16) & 0x1F;
  u32 rs = (instr >> 21) & 0x1F;
  i16 offset = static_cast<i16>(instr & 0xFFFF);

  u32 addr = gpr_[rs] + static_cast<u32>(offset);
  u32 value = bus_->read32(addr);

  fpr_[ft] = value;
}

void MIPSR5000::execute_swc1(u32 instr) {
  u32 ft = (instr >> 16) & 0x1F;
  u32 rs = (instr >> 21) & 0x1F;
  i16 offset = static_cast<i16>(instr & 0xFFFF);

  u32 addr = gpr_[rs] + static_cast<u32>(offset);
  u32 value = fpr_[ft];

  bus_->write32(addr, value);
}

void MIPSR5000::exception(u32 exc_code, u32 bad_addr) {
  // Save current PC to EPC
  if (cp0_[CP0_STATUS] & STATUS_EXL) {
    // Nested exception - use ErrorEPC
    cp0_[CP0_ERROREPC] = pc_;
  } else {
    cp0_[CP0_EPC] = pc_;
    cp0_[CP0_STATUS] |= STATUS_EXL;
  }

  // Set cause
  cp0_[CP0_CAUSE] = (cp0_[CP0_CAUSE] & ~CAUSE_EXCMASK) | (exc_code << 2);

  // Set bad address for address errors
  if (exc_code == EXC_ADEL || exc_code == EXC_ADES) {
    cp0_[CP0_BADVADDR] = bad_addr;
  }

  // Set BD bit if in branch delay slot
  if (branch_delay_) {
    cp0_[CP0_CAUSE] |= CAUSE_BD;
  }

  // Vector to exception handler
  if (cp0_[CP0_STATUS] & STATUS_BEV) {
    pc_ = 0xBFC00200; // Bootstrap exception vector
  } else {
    pc_ = 0x80000000; // General exception vector
  }
  next_pc_ = pc_ + 4;
  branch_delay_ = false;
}

void MIPSR5000::check_interrupts() {
  u32 status = cp0_[CP0_STATUS];
  u32 cause = cp0_[CP0_CAUSE];

  // Check if interrupts are enabled
  if (!(status & STATUS_IE))
    return;
  if (status & STATUS_EXL)
    return;
  if (status & STATUS_ERL)
    return;

  // Check pending interrupts
  u32 pending = cause & status & STATUS_IM;
  if (pending) {
    // Find highest priority interrupt
    for (int i = 0; i < 8; ++i) {
      if (pending & (1 << (8 + i))) {
        exception(EXC_INT, 0);
        return;
      }
    }
  }
}

u32 MIPSR5000::gpr(int reg) const {
  if (reg >= 0 && reg < 32)
    return gpr_[reg];
  return 0;
}

void MIPSR5000::set_gpr(int reg, u32 value) {
  if (reg >= 0 && reg < 32)
    gpr_[reg] = value;
}

u32 MIPSR5000::cp0_reg(int reg) const {
  if (reg >= 0 && reg < 32)
    return cp0_[reg];
  return 0;
}

void MIPSR5000::set_cp0_reg(int reg, u32 value) {
  if (reg >= 0 && reg < 32)
    cp0_[reg] = value;
}

u64 MIPSR5000::cycles() const { return cycles_; }

u64 MIPSR5000::instructions() const { return instr_count_; }

} // namespace o2emu::cpu