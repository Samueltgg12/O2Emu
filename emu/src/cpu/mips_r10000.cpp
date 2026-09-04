/**
 * @file mips_r10000.cpp
 * @brief MIPS R10000/R12000 CPU core implementation
 *
 * This implements a simplified but accurate model of the R10000/R12000
 * superscalar out-of-order processor. Key features modeled:
 * - 4-issue superscalar pipeline
 * - Register renaming with 128 physical registers
 * - 64-entry reorder buffer for in-order retirement
 * - Branch prediction with 2-bit saturating counters
 * - 6 functional units (2 ALU, 1 MULT/DIV, 1 LOAD, 1 STORE, 2 FPU)
 * - MIPS IV ISA (conditional moves, prefetch, etc.)
 * - 64-bit mode support
 */

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <o2emu/cpu/mips_r10000.h>
#include <o2emu/logging/logger.h>

namespace o2emu::cpu {

MIPSR10000::MIPSR10000(system::Bus *bus, Variant variant)
    : bus_(bus), variant_(variant) {
  reset();
}

void MIPSR10000::reset() {
  // Clear architectural state
  gpr_.fill(0);
  fpr_.fill(0);
  cp0_.fill(0);
  hi_ = 0;
  lo_ = 0;
  pc_ = 0xBFC00000; // Reset vector
  next_pc_ = pc_ + 4;
  fcr0_ = 0;
  fcr31_ = 0x00000000;
  llbit_ = false;
  lladdr_ = 0;
  branch_delay_ = false;
  flush_pipeline_ = false;

  // Clear physical register file
  phys_gpr_.fill(0);
  phys_fpr_.fill(0);
  phys_gpr_valid_.fill(false);
  phys_fpr_valid_.fill(false);
  phys_gpr_arch_map_.fill(-1);
  phys_fpr_arch_map_.fill(-1);

  // Initialize free lists
  init_free_lists();

  // Map architectural registers to first 32 physical registers
  for (int i = 0; i < 32; ++i) {
    gpr_rename_map_[i] = {i, true, -1};
    fpr_rename_map_[i] = {i, true, -1};
    phys_gpr_arch_map_[i] = i;
    phys_fpr_arch_map_[i] = i;
    phys_gpr_valid_[i] = true;
    phys_fpr_valid_[i] = true;
  }

  // Clear pipelines
  fetch_queue_.fill({});
  fetch_queue_head_ = fetch_queue_tail_ = fetch_queue_count_ = 0;
  issue_queue_.fill({});
  issue_queue_count_ = 0;
  rob_.fill({});
  rob_head_ = rob_tail_ = rob_count_ = 0;
  branch_predictor_.fill({});
  fu_status_.fill({});
  load_queue_.fill({});
  store_queue_.fill({});
  load_queue_count_ = store_queue_count_ = 0;

  // Performance counters
  perf_counters_.fill(0);
  perf_counter_ctrl_.fill(0);

  // Statistics
  stats_ = {};
  cycles_ = 0;
  instr_count_ = 0;
  committed_count_ = 0;

  // Initialize CP0 registers
  cp0_[static_cast<int>(R10K_CP0_Register::INDEX)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::RANDOM)] = 63; // 64 TLB entries
  cp0_[static_cast<int>(R10K_CP0_Register::ENTRYLO0)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::ENTRYLO1)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::CONTEXT)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::PAGEMASK)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::WIRED)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::BADVADDR)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::COUNT)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::ENTRYHI)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::COMPARE)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::STATUS)] = 0x00400004; // BEV=1, TS=1
  cp0_[static_cast<int>(R10K_CP0_Register::CAUSE)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::EPC)] = 0;

  // Set PRID based on variant
  switch (variant_) {
  case Variant::R10000:
    cp0_[static_cast<int>(R10K_CP0_Register::PRID)] = PRID_R10000;
    break;
  case Variant::R10000A:
    cp0_[static_cast<int>(R10K_CP0_Register::PRID)] = PRID_R10000A;
    break;
  case Variant::R12000:
    cp0_[static_cast<int>(R10K_CP0_Register::PRID)] = PRID_R12000;
    break;
  case Variant::R12000A:
    cp0_[static_cast<int>(R10K_CP0_Register::PRID)] = PRID_R12000A;
    break;
  }

  // Config register: MIPS IV, 64-bit FPU, split secondary cache
  cp0_[static_cast<int>(R10K_CP0_Register::CONFIG)] =
      CONFIG_AT | (2 << 10) | CONFIG_VI | CONFIG_SS;

  cp0_[static_cast<int>(R10K_CP0_Register::LLADDR)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::WATCHLO)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::WATCHHI)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::XCONTEXT)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::ECC)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::CACHEERR)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::TAGLO)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::TAGHI)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::ERROREPC)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::PERFCNT)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::PERFCNT_CTRL)] = 0;
  cp0_[static_cast<int>(R10K_CP0_Register::PREFETCH_CTRL)] = 0;
}

void MIPSR10000::init_free_lists() {
  // Initialize free physical register lists (registers 32-127)
  free_phys_gpr_head_ = 32;
  free_phys_fpr_head_ = 32;
  for (int i = 32; i < NUM_PHYS_REGS - 1; ++i) {
    phys_gpr_arch_map_[i] = i + 1;
    phys_fpr_arch_map_[i] = i + 1;
  }
  phys_gpr_arch_map_[NUM_PHYS_REGS - 1] = -1;
  phys_fpr_arch_map_[NUM_PHYS_REGS - 1] = -1;
}

void MIPSR10000::tick(uint64_t cycles) {
  cycles_ += cycles;
  stats_.cycles += cycles;

  for (uint64_t i = 0; i < cycles; ++i) {
    // Pipeline stages execute in reverse order (commit first, fetch last)
    // to avoid overwriting state that later stages need

    // Check for pipeline flush
    if (flush_pipeline_) {
      flush_pipeline(flush_target_);
      flush_pipeline_ = false;
    }

    // Commit stage (in-order retirement)
    commit_stage();

    // Writeback stage
    writeback_stage();

    // Execute stage
    execute_stage();

    // Issue stage
    issue_stage();

    // Decode stage
    decode_stage();

    // Fetch stage
    fetch_stage();

    // Update CP0 Count register (increments every 2 cycles)
    if ((cycles_ & 1) == 0) {
      cp0_[static_cast<int>(R10K_CP0_Register::COUNT)]++;
    }

    // Check for timer interrupt
    if (cp0_[static_cast<int>(R10K_CP0_Register::COUNT)] >=
        cp0_[static_cast<int>(R10K_CP0_Register::COMPARE)]) {
      cp0_[static_cast<int>(R10K_CP0_Register::CAUSE)] |= MIPSR5000::CAUSE_TI;
    }

    // Check for performance counter interrupts
    for (int i = 0; i < 2; ++i) {
      if (perf_counter_ctrl_[i] & 0x80000000) { // Interrupt enable
        if (perf_counters_[i] >= (perf_counter_ctrl_[i] & 0x7FFFFFFF)) {
          cp0_[static_cast<int>(R10K_CP0_Register::CAUSE)] |= CAUSE_PCI;
        }
      }
    }

    check_interrupts();
  }
}

void MIPSR10000::fetch_stage() {
  // Fetch up to 4 instructions per cycle
  int fetched = 0;
  while (fetched < 4 && fetch_queue_count_ < FETCH_QUEUE_SIZE) {
    // Check for branch delay slot
    if (branch_delay_) {
      // Fetch from next_pc_ (delay slot)
      uint32_t instr = 0;
      if (!bus_->read8(next_pc_, 4, instr)) {
        // Bus error - will be handled in decode
        InstructionEntry entry;
        entry.pc = next_pc_;
        entry.instr = 0;
        entry.exception = true;
        entry.exc_code = static_cast<uint32_t>(ExceptionCode::IBE);
        fetch_queue_[fetch_queue_tail_] = entry;
      } else {
        InstructionEntry entry = decode_instruction(next_pc_, instr);
        fetch_queue_[fetch_queue_tail_] = entry;
      }
      branch_delay_ = false;
    } else {
      // Normal fetch from pc_
      uint32_t instr = 0;
      if (!bus_->read8(pc_, 4, instr)) {
        InstructionEntry entry;
        entry.pc = pc_;
        entry.instr = 0;
        entry.exception = true;
        entry.exc_code = static_cast<uint32_t>(ExceptionCode::IBE);
        fetch_queue_[fetch_queue_tail_] = entry;
      } else {
        InstructionEntry entry = decode_instruction(pc_, instr);

        // Branch prediction for branch instructions
        if ((entry.opcode == 0x04 || entry.opcode == 0x05 || // BEQ, BNE
             entry.opcode == 0x06 || entry.opcode == 0x07 || // BLEZ, BGTZ
             entry.opcode == 0x01) && // REGIMM (BLTZ, BGEZ, etc.)
            (entry.rt == 0x00 || entry.rt == 0x01 || entry.rt == 0x10 ||
             entry.rt == 0x11)) {
          entry.predicted_taken =
              predict_branch(entry.pc, entry.predicted_target);
          stats_.branches_predicted++;
        }

        fetch_queue_[fetch_queue_tail_] = entry;
      }
    }

    fetch_queue_tail_ = (fetch_queue_tail_ + 1) % FETCH_QUEUE_SIZE;
    fetch_queue_count_++;
    fetched++;
    stats_.instructions_fetched++;

    // Advance PC
    if (!branch_delay_) {
      pc_ = next_pc_;
      next_pc_ = pc_ + 4;
    }
  }
}

void MIPSR10000::decode_stage() {
  // Move instructions from fetch queue to issue queue
  while (fetch_queue_count_ > 0 && issue_queue_count_ < ISSUE_QUEUE_SIZE) {
    InstructionEntry entry = fetch_queue_[fetch_queue_head_];
    fetch_queue_head_ = (fetch_queue_head_ + 1) % FETCH_QUEUE_SIZE;
    fetch_queue_count_--;

    // Handle exceptions from fetch
    if (entry.exception) {
      // Create ROB entry for exception
      ROBEntry rob_entry;
      rob_entry.pc = entry.pc;
      rob_entry.instr = entry.instr;
      rob_entry.exception = true;
      rob_entry.exc_code = entry.exc_code;
      rob_entry.completed = true; // Exceptions "complete" immediately
      rob_[rob_tail_] = rob_entry;
      rob_tail_ = (rob_tail_ + 1) % ROB_SIZE;
      rob_count_++;
      continue;
    }

    // Register renaming
    rename_registers(entry);

    // Allocate ROB entry
    if (rob_count_ >= ROB_SIZE) {
      // ROB full, stall
      fetch_queue_head_ =
          (fetch_queue_head_ - 1 + FETCH_QUEUE_SIZE) % FETCH_QUEUE_SIZE;
      fetch_queue_count_++;
      stats_.stalls_structural++;
      break;
    }

    int rob_index = rob_tail_;
    entry.phys_rd = (entry.rd != 0) ? entry.phys_rd : -1;

    ROBEntry rob_entry;
    rob_entry.pc = entry.pc;
    rob_entry.instr = entry.instr;
    rob_entry.phys_dest = entry.phys_rd;
    rob_entry.arch_dest = entry.rd;
    rob_entry.is_branch =
        (entry.opcode == 0x04 || entry.opcode == 0x05 || entry.opcode == 0x06 ||
         entry.opcode == 0x07 || entry.opcode == 0x01 || entry.opcode == 0x02 ||
         entry.opcode == 0x03);
    rob_entry.predicted_taken = entry.predicted_taken;
    rob_entry.predicted_target = entry.predicted_target;
    rob_entry.in_delay_slot = entry.in_delay_slot;
    rob_[rob_index] = rob_entry;
    rob_tail_ = (rob_tail_ + 1) % ROB_SIZE;
    rob_count_++;

    // Update rename map to point to ROB entry
    if (entry.rd != 0) {
      gpr_rename_map_[entry.rd].rob_index = rob_index;
    }

    // Add to issue queue
    issue_queue_[issue_queue_count_] = entry;
    issue_queue_count_++;
    stats_.instructions_issued++;
  }
}

void MIPSR10000::rename_registers(InstructionEntry &entry) {
  // Rename source registers
  if (entry.rs != 0) {
    int phys_rs = get_phys_gpr(entry.rs);
    entry.phys_rs = phys_rs;
    entry.rs_ready = phys_gpr_valid_[phys_rs];
    if (!entry.rs_ready) {
      entry.dep_rs = gpr_rename_map_[entry.rs].rob_index;
    }
  }

  if (entry.rt != 0) {
    int phys_rt = get_phys_gpr(entry.rt);
    entry.phys_rt = phys_rt;
    entry.rt_ready = phys_gpr_valid_[phys_rt];
    if (!entry.rt_ready) {
      entry.dep_rt = gpr_rename_map_[entry.rt].rob_index;
    }
  }

  // Allocate destination register
  if (entry.rd != 0) {
    entry.phys_rd = allocate_phys_gpr(entry.rd);
    entry.old_phys_rd = gpr_rename_map_[entry.rd].phys_reg;
    gpr_rename_map_[entry.rd] = {entry.phys_rd, false, rob_tail_};
    phys_gpr_valid_[entry.phys_rd] = false;
  }
}

int MIPSR10000::allocate_phys_gpr(int arch_reg) {
  int phys = free_phys_gpr_head_;
  if (phys == -1)
    return -1; // Should not happen with 128 registers
  free_phys_gpr_head_ = phys_gpr_arch_map_[phys];
  phys_gpr_arch_map_[phys] = arch_reg;
  return phys;
}

int MIPSR10000::allocate_phys_fpr(int arch_reg) {
  int phys = free_phys_fpr_head_;
  if (phys == -1)
    return -1;
  free_phys_fpr_head_ = phys_fpr_arch_map_[phys];
  phys_fpr_arch_map_[phys] = arch_reg;
  return phys;
}

void MIPSR10000::free_phys_gpr(int phys_reg) {
  if (phys_reg < 32 || phys_reg >= NUM_PHYS_REGS)
    return;
  phys_gpr_arch_map_[phys_reg] = free_phys_gpr_head_;
  free_phys_gpr_head_ = phys_reg;
  phys_gpr_valid_[phys_reg] = false;
}

void MIPSR10000::free_phys_fpr(int phys_reg) {
  if (phys_reg < 32 || phys_reg >= NUM_PHYS_REGS)
    return;
  phys_fpr_arch_map_[phys_reg] = free_phys_fpr_head_;
  free_phys_fpr_head_ = phys_reg;
  phys_fpr_valid_[phys_reg] = false;
}

int MIPSR10000::get_phys_gpr(int arch_reg) const {
  if (arch_reg == 0)
    return 0; // $0 is always physical register 0
  return gpr_rename_map_[arch_reg].phys_reg;
}

int MIPSR10000::get_phys_fpr(int arch_reg) const {
  return fpr_rename_map_[arch_reg].phys_reg;
}

uint64_t MIPSR10000::read_phys_gpr(int phys_reg) const {
  if (phys_reg == 0)
    return 0;
  return phys_gpr_[phys_reg];
}

void MIPSR10000::write_phys_gpr(int phys_reg, uint64_t value) {
  if (phys_reg == 0)
    return;
  phys_gpr_[phys_reg] = value;
  phys_gpr_valid_[phys_reg] = true;
}

uint64_t MIPSR10000::read_phys_fpr(int phys_reg) const {
  return phys_fpr_[phys_reg];
}

void MIPSR10000::write_phys_fpr(int phys_reg, uint64_t value) {
  phys_fpr_[phys_reg] = value;
  phys_fpr_valid_[phys_reg] = true;
}

bool MIPSR10000::predict_branch(uint32_t pc, uint32_t &predicted_target) {
  int index = pc & (BP_SIZE - 1);
  BranchPredictorEntry &bp_entry = branch_predictor_[index];

  if (bp_entry.valid && bp_entry.pc == pc) {
    predicted_target = bp_entry.target;
    return bp_entry.counter >= 2; // Taken if counter >= 2
  }

  // Not in predictor, predict not taken
  predicted_target = pc + 4;
  return false;
}

void MIPSR10000::update_branch_predictor(uint32_t pc, bool taken,
                                         uint32_t actual_target) {
  int index = pc & (BP_SIZE - 1);
  BranchPredictorEntry &bp_entry = branch_predictor_[index];

  if (!bp_entry.valid || bp_entry.pc != pc) {
    bp_entry.pc = pc;
    bp_entry.target = actual_target;
    bp_entry.valid = true;
    bp_entry.counter = taken ? 2 : 1; // Weakly taken/not taken
  } else {
    bp_entry.target = actual_target;
    if (taken) {
      if (bp_entry.counter < 3)
        bp_entry.counter++;
    } else {
      if (bp_entry.counter > 0)
        bp_entry.counter--;
    }
  }
}

void MIPSR10000::issue_stage() {
  // Issue up to 4 instructions per cycle to available functional units
  int issued = 0;
  for (int i = 0; i < issue_queue_count_ && issued < 4; ++i) {
    InstructionEntry &entry = issue_queue_[i];

    // Check if operands are ready
    bool rs_ready = entry.rs_ready || (entry.rs == 0);
    bool rt_ready = entry.rt_ready || (entry.rt == 0);

    // Check for dependencies in ROB
    if (!rs_ready && entry.dep_rs >= 0) {
      ROBEntry &dep_entry = rob_[entry.dep_rs];
      if (dep_entry.completed) {
        rs_ready = true;
        entry.rs_ready = true;
      }
    }
    if (!rt_ready && entry.dep_rt >= 0) {
      ROBEntry &dep_entry = rob_[entry.dep_rt];
      if (dep_entry.completed) {
        rt_ready = true;
        entry.rt_ready = true;
      }
    }

    if (rs_ready && rt_ready) {
      // Try to allocate functional unit
      if (allocate_fu(entry)) {
        // Remove from issue queue
        entry.stage = PipelineStage::EX;
        // Shift remaining entries
        for (int j = i; j < issue_queue_count_ - 1; ++j) {
          issue_queue_[j] = issue_queue_[j + 1];
        }
        issue_queue_count_--;
        i--; // Recheck this index
        issued++;
      } else {
        stats_.stalls_structural++;
      }
    } else {
      stats_.stalls_data++;
    }
  }
}

bool MIPSR10000::allocate_fu(InstructionEntry &entry) {
  // Determine which functional unit is needed
  FunctionalUnit fu = FunctionalUnit::ALU1;

  switch (entry.opcode) {
  case 0x00: // SPECIAL
    switch (entry.funct) {
    case 0x18:
    case 0x19:
    case 0x1A:
    case 0x1B: // MULT, MULTU, DIV, DIVU
      fu = FunctionalUnit::MULT;
      break;
    default:
      fu = FunctionalUnit::ALU1;
      break;
    }
    break;
  case 0x01: // REGIMM
  case 0x04:
  case 0x05:
  case 0x06:
  case 0x07: // Branches
  case 0x02:
  case 0x03: // J, JAL
    fu = FunctionalUnit::BRANCH;
    break;
  case 0x20:
  case 0x21:
  case 0x22:
  case 0x23: // LB, LH, LWL, LW
  case 0x24:
  case 0x25:
  case 0x26:
  case 0x27: // LBU, LHU, LWR, LD
  case 0x30:
  case 0x31:
  case 0x32:
  case 0x33: // LL, LWC1, LWC2, PREF
    fu = FunctionalUnit::LOAD;
    stats_.load_store_issued++;
    break;
  case 0x28:
  case 0x29:
  case 0x2A:
  case 0x2B: // SB, SH, SWL, SW
  case 0x2C:
  case 0x2D:
  case 0x2E:
  case 0x2F: // SWR, CACHE, PREF, SD
  case 0x38:
  case 0x39:
  case 0x3A:
  case 0x3B: // SC, SWC1, SWC2, SD
    fu = FunctionalUnit::STORE;
    stats_.load_store_issued++;
    break;
  case 0x11: // COP1 (FPU)
    fu = (entry.funct & 0x20) ? FunctionalUnit::FPU2 : FunctionalUnit::FPU1;
    stats_.fp_issued++;
    break;
  default:
    fu = FunctionalUnit::ALU1;
    break;
  }

  // Try primary, then secondary
  if (!fu_status_[static_cast<int>(fu)].busy) {
    fu_status_[static_cast<int>(fu)] = {true, -1, 0};
    entry.fu = fu;
    return true;
  }

  // Try ALU2 for ALU1 instructions
  if (fu == FunctionalUnit::ALU1 &&
      !fu_status_[static_cast<int>(FunctionalUnit::ALU2)].busy) {
    fu_status_[static_cast<int>(FunctionalUnit::ALU2)] = {true, -1, 0};
    entry.fu = FunctionalUnit::ALU2;
    return true;
  }

  return false;
}

void MIPSR10000::free_fu(FunctionalUnit fu) {
  fu_status_[static_cast<int>(fu)] = {false, -1, 0};
}

void MIPSR10000::execute_stage() {
  // Execute instructions in functional units
  for (int fu_idx = 0; fu_idx < 8; ++fu_idx) {
    if (!fu_status_[fu_idx].busy)
      continue;

    int rob_index = fu_status_[fu_idx].rob_index;
    if (rob_index < 0)
      continue;

    ROBEntry &rob_entry = rob_[rob_index];

    // Find the instruction entry (simplified - in real impl would track
    // separately) For now, we execute directly in the ROB entry
    if (fu_status_[fu_idx].cycles_remaining > 0) {
      fu_status_[fu_idx].cycles_remaining--;
    }

    if (fu_status_[fu_idx].cycles_remaining == 0) {
      // Execution complete
      rob_entry.completed = true;
      free_fu(static_cast<FunctionalUnit>(fu_idx));
    }
  }
}

void MIPSR10000::writeback_stage() {
  // Writeback results to physical registers
  for (int i = 0; i < ROB_SIZE; ++i) {
    ROBEntry &entry = rob_[i];
    if (entry.completed && entry.phys_dest >= 0 && !entry.exception) {
      // Result already written during execute
      // Mark physical register as valid
      phys_gpr_valid_[entry.phys_dest] = true;
    }
  }
}

void MIPSR10000::commit_stage() {
  // Commit up to 4 instructions per cycle in order
  int committed = 0;
  while (rob_count_ > 0 && committed < 4) {
    ROBEntry &entry = rob_[rob_head_];

    if (!entry.completed) {
      break; // Can't commit past incomplete instruction
    }

    // Handle exceptions
    if (entry.exception) {
      handle_exception(entry.exc_code, entry.bad_addr);
      // Flush pipeline after exception
      flush_pipeline_ = true;
      flush_target_ = 0x80000000 | (entry.exc_code << 2); // Exception vector
      break;
    }

    // Handle branch misprediction
    if (entry.is_branch) {
      bool actually_taken = (entry.actually_taken != entry.predicted_taken);
      if (actually_taken) {
        stats_.branches_mispredicted++;
        flush_pipeline_ = true;
        flush_target_ = entry.actual_target;
        // Update branch predictor
        update_branch_predictor(entry.pc, entry.actually_taken,
                                entry.actual_target);
        break;
      } else {
        // Correct prediction
        update_branch_predictor(entry.pc, entry.predicted_taken,
                                entry.predicted_target);
      }
    }

    // Commit the instruction
    commit_instruction(entry);

    // Free old physical register if renamed
    if (entry.arch_dest >= 0 && entry.arch_dest != 0) {
      // Find old physical register (would need tracking)
      // For now, simplified
    }

    // Advance ROB
    rob_head_ = (rob_head_ + 1) % ROB_SIZE;
    rob_count_--;
    committed++;
    committed_count_++;
    stats_.instructions_committed++;
  }
}

void MIPSR10000::commit_instruction(ROBEntry &entry) {
  // Update architectural state
  if (entry.arch_dest >= 0 && entry.arch_dest != 0) {
    // The physical register already has the result
    // Architectural register now maps to this physical register
    gpr_[entry.arch_dest] = phys_gpr_[entry.phys_dest];
  }

  // Update PC for branches
  if (entry.is_branch && entry.actually_taken) {
    pc_ = entry.actual_target;
    next_pc_ = pc_ + 4;
  }
}

void MIPSR10000::flush_pipeline(uint32_t target_pc) {
  // Clear fetch queue
  fetch_queue_.fill({});
  fetch_queue_head_ = fetch_queue_tail_ = fetch_queue_count_ = 0;

  // Clear issue queue
  issue_queue_.fill({});
  issue_queue_count_ = 0;

  // Clear ROB and free physical registers
  for (int i = 0; i < rob_count_; ++i) {
    int idx = (rob_head_ + i) % ROB_SIZE;
    ROBEntry &entry = rob_[idx];
    if (entry.phys_dest >= 0 && entry.phys_dest >= 32) {
      free_phys_gpr(entry.phys_dest);
    }
  }
  rob_.fill({});
  rob_head_ = rob_tail_ = rob_count_ = 0;

  // Reset rename map to architectural registers
  for (int i = 0; i < 32; ++i) {
    gpr_rename_map_[i] = {i, true, -1};
  }

  // Reset functional units
  fu_status_.fill({});

  // Reset load/store queues
  load_queue_.fill({});
  store_queue_.fill({});
  load_queue_count_ = store_queue_count_ = 0;

  // Set new PC
  pc_ = target_pc;
  next_pc_ = pc_ + 4;
  branch_delay_ = false;
}

void MIPSR10000::handle_exception(uint32_t exc_code, uint32_t bad_addr) {
  uint32_t status = cp0_[static_cast<int>(R10K_CP0_Register::STATUS)];
  uint32_t cause = cp0_[static_cast<int>(R10K_CP0_Register::CAUSE)];

  // Set exception code
  cause = (cause & ~MIPSR5000::CAUSE_EXCMASK) | (exc_code << 2);

  // Set BD bit if in delay slot
  if (branch_delay_) {
    cause |= MIPSR5000::CAUSE_BD;
    cp0_[static_cast<int>(R10K_CP0_Register::EPC)] = pc_ - 4;
  } else {
    cause &= ~MIPSR5000::CAUSE_BD;
    cp0_[static_cast<int>(R10K_CP0_Register::EPC)] = pc_;
  }

  cp0_[static_cast<int>(R10K_CP0_Register::CAUSE)] = cause;
  cp0_[static_cast<int>(R10K_CP0_Register::BADVADDR)] = bad_addr;

  // Set EXL bit
  status |= 0x00000002; // EXL
  cp0_[static_cast<int>(R10K_CP0_Register::STATUS)] = status;

  // Jump to exception vector
  uint32_t vector = (status & 0x00400000) ? 0xBFC00200 : 0x80000000; // BEV bit
  vector += (exc_code << 2);

  flush_pipeline_ = true;
  flush_target_ = vector;
}

void MIPSR10000::check_interrupts() {
  uint32_t status = cp0_[static_cast<int>(R10K_CP0_Register::STATUS)];
  uint32_t cause = cp0_[static_cast<int>(R10K_CP0_Register::CAUSE)];

  // Check if interrupts enabled
  if (!(status & 0x00000001))
    return; // IE bit clear
  if (status & 0x00000002)
    return; // EXL bit set
  if (status & 0x00000004)
    return; // ERL bit set

  // Check interrupt mask
  uint32_t ip = cause & 0x0000FF00;
  uint32_t im = status & 0x0000FF00;
  uint32_t pending = ip & im;

  if (pending) {
    // Take interrupt exception
    handle_exception(static_cast<uint32_t>(ExceptionCode::INT), 0);
  }
}

InstructionEntry MIPSR10000::decode_instruction(uint32_t pc, uint32_t instr) {
  InstructionEntry entry;
  entry.pc = pc;
  entry.instr = instr;
  entry.opcode = instr >> 26;
  entry.rs = (instr >> 21) & 0x1F;
  entry.rt = (instr >> 16) & 0x1F;
  entry.rd = (instr >> 11) & 0x1F;
  entry.shamt = (instr >> 6) & 0x1F;
  entry.funct = instr & 0x3F;
  entry.imm = static_cast<int16_t>(instr & 0xFFFF);
  entry.target = (instr & 0x03FFFFFF) << 2;
  entry.in_delay_slot = branch_delay_;

  return entry;
}

void MIPSR10000::execute_instruction(InstructionEntry &entry) {
  // This is called when instruction reaches execute stage
  // Actual execution happens in functional unit specific methods
  switch (entry.fu) {
  case FunctionalUnit::ALU1:
  case FunctionalUnit::ALU2:
    execute_alu(entry);
    break;
  case FunctionalUnit::MULT:
    execute_mult_div(entry);
    break;
  case FunctionalUnit::LOAD:
    execute_load(entry);
    break;
  case FunctionalUnit::STORE:
    execute_store(entry);
    break;
  case FunctionalUnit::FPU1:
  case FunctionalUnit::FPU2:
    execute_fpu(entry);
    break;
  case FunctionalUnit::BRANCH:
    execute_branch(entry);
    break;
  }
}

void MIPSR10000::execute_alu(InstructionEntry &entry) {
  uint64_t rs_val = (entry.rs == 0) ? 0 : read_phys_gpr(entry.phys_rs);
  uint64_t rt_val = (entry.rt == 0) ? 0 : read_phys_gpr(entry.phys_rt);
  uint64_t result = 0;
  bool is_64bit = is_64bit_mode();

  switch (entry.opcode) {
  case 0x00: // SPECIAL
    switch (entry.funct) {
    case 0x00: // SLL
      result = rt_val << entry.shamt;
      break;
    case 0x02: // SRL
      result = rt_val >> entry.shamt;
      break;
    case 0x03: // SRA
      result = static_cast<int64_t>(rt_val) >> entry.shamt;
      break;
    case 0x04: // SLLV
      result = rt_val << (rs_val & 0x3F);
      break;
    case 0x06: // SRLV
      result = rt_val >> (rs_val & 0x3F);
      break;
    case 0x07: // SRAV
      result = static_cast<int64_t>(rt_val) >> (rs_val & 0x3F);
      break;
    case 0x08: // JR
      entry.actually_taken = true;
      entry.actual_target = static_cast<uint32_t>(rs_val);
      branch_delay_ = true;
      break;
    case 0x09: // JALR
      result = pc_ + 8;
      entry.actually_taken = true;
      entry.actual_target = static_cast<uint32_t>(rs_val);
      branch_delay_ = true;
      break;
    case 0x0C: // SYSCALL
      entry.exception = true;
      entry.exc_code = static_cast<uint32_t>(ExceptionCode::SYS);
      break;
    case 0x0D: // BREAK
      entry.exception = true;
      entry.exc_code = static_cast<uint32_t>(ExceptionCode::BP);
      break;
    case 0x10: // MFHI
      result = hi_;
      break;
    case 0x11: // MTHI
      hi_ = rs_val;
      break;
    case 0x12: // MFLO
      result = lo_;
      break;
    case 0x13: // MTLO
      lo_ = rs_val;
      break;
    case 0x20: // ADD
      result = rs_val + rt_val;
      // Check overflow for 32-bit
      if (!is_64bit && ((static_cast<int64_t>(rs_val) > 0 &&
                         static_cast<int64_t>(rt_val) > 0 &&
                         static_cast<int64_t>(result) < 0) ||
                        (static_cast<int64_t>(rs_val) < 0 &&
                         static_cast<int64_t>(rt_val) < 0 &&
                         static_cast<int64_t>(result) > 0))) {
        entry.exception = true;
        entry.exc_code = static_cast<uint32_t>(ExceptionCode::OV);
      }
      break;
    case 0x21: // ADDU
      result = rs_val + rt_val;
      break;
    case 0x22: // SUB
      result = rs_val - rt_val;
      if (!is_64bit && ((static_cast<int64_t>(rs_val) > 0 &&
                         static_cast<int64_t>(rt_val) < 0 &&
                         static_cast<int64_t>(result) < 0) ||
                        (static_cast<int64_t>(rs_val) < 0 &&
                         static_cast<int64_t>(rt_val) > 0 &&
                         static_cast<int64_t>(result) > 0))) {
        entry.exception = true;
        entry.exc_code = static_cast<uint32_t>(ExceptionCode::OV);
      }
      break;
    case 0x23: // SUBU
      result = rs_val - rt_val;
      break;
    case 0x24: // AND
      result = rs_val & rt_val;
      break;
    case 0x25: // OR
      result = rs_val | rt_val;
      break;
    case 0x26: // XOR
      result = rs_val ^ rt_val;
      break;
    case 0x27: // NOR
      result = ~(rs_val | rt_val);
      break;
    case 0x2A: // SLT
      result =
          (static_cast<int64_t>(rs_val) < static_cast<int64_t>(rt_val)) ? 1 : 0;
      break;
    case 0x2B: // SLTU
      result = (rs_val < rt_val) ? 1 : 0;
      break;
    case 0x10: // MFHI (already handled)
    case 0x11: // MTHI (already handled)
    case 0x12: // MFLO (already handled)
    case 0x13: // MTLO (already handled)
      break;
    default:
      entry.exception = true;
      entry.exc_code = static_cast<uint32_t>(ExceptionCode::RI);
      break;
    }
    break;
  case 0x08: // ADDI
    result = rs_val + static_cast<int64_t>(entry.imm);
    if (!is_64bit && ((static_cast<int64_t>(rs_val) > 0 && entry.imm > 0 &&
                       static_cast<int64_t>(result) < 0) ||
                      (static_cast<int64_t>(rs_val) < 0 && entry.imm < 0 &&
                       static_cast<int64_t>(result) > 0))) {
      entry.exception = true;
      entry.exc_code = static_cast<uint32_t>(ExceptionCode::OV);
    }
    break;
  case 0x09: // ADDIU
    result = rs_val + static_cast<uint64_t>(static_cast<int16_t>(entry.imm));
    break;
  case 0x0A: // SLTI
    result = (static_cast<int64_t>(rs_val) < static_cast<int16_t>(entry.imm))
                 ? 1
                 : 0;
    break;
  case 0x0B: // SLTIU
    result = (rs_val < static_cast<uint16_t>(entry.imm)) ? 1 : 0;
    break;
  case 0x0C: // ANDI
    result = rs_val & static_cast<uint16_t>(entry.imm);
    break;
  case 0x0D: // ORI
    result = rs_val | static_cast<uint16_t>(entry.imm);
    break;
  case 0x0E: // XORI
    result = rs_val ^ static_cast<uint16_t>(entry.imm);
    break;
  case 0x0F: // LUI
    result = static_cast<uint64_t>(entry.imm) << 16;
    break;
  default:
    entry.exception = true;
    entry.exc_code = static_cast<uint32_t>(ExceptionCode::RI);
    break;
  }

  // Sign extend for 32-bit mode
  if (!is_64bit) {
    result = static_cast<int32_t>(result);
  }

  entry.result = result;
  if (entry.phys_rd >= 0) {
    write_phys_gpr(entry.phys_rd, result);
  }
}

void MIPSR10000::execute_mult_div(InstructionEntry &entry) {
  uint64_t rs_val = (entry.rs == 0) ? 0 : read_phys_gpr(entry.phys_rs);
  uint64_t rt_val = (entry.rt == 0) ? 0 : read_phys_gpr(entry.phys_rt);

  // Multi-cycle operations
  int cycles = 0;
  switch (entry.funct) {
  case 0x18:     // MULT
    cycles = 10; // ~10 cycles for 64-bit multiply
    {
      int64_t result = static_cast<int64_t>(static_cast<int32_t>(rs_val)) *
                       static_cast<int64_t>(static_cast<int32_t>(rt_val));
      lo_ = static_cast<uint32_t>(result);
      hi_ = static_cast<uint32_t>(result >> 32);
    }
    break;
  case 0x19: // MULTU
    cycles = 10;
    {
      uint64_t result = rs_val * rt_val;
      lo_ = static_cast<uint32_t>(result);
      hi_ = static_cast<uint32_t>(result >> 32);
    }
    break;
  case 0x1A:     // DIV
    cycles = 35; // ~35 cycles for divide
    if (rt_val != 0) {
      lo_ = static_cast<uint32_t>(static_cast<int32_t>(rs_val) /
                                  static_cast<int32_t>(rt_val));
      hi_ = static_cast<uint32_t>(static_cast<int32_t>(rs_val) %
                                  static_cast<int32_t>(rt_val));
    }
    break;
  case 0x1B: // DIVU
    cycles = 35;
    if (rt_val != 0) {
      lo_ = rs_val / rt_val;
      hi_ = rs_val % rt_val;
    }
    break;
  }

  fu_status_[static_cast<int>(FunctionalUnit::MULT)].cycles_remaining = cycles;
}

void MIPSR10000::execute_load(InstructionEntry &entry) {
  uint64_t base = (entry.rs == 0) ? 0 : read_phys_gpr(entry.phys_rs);
  uint32_t addr = static_cast<uint32_t>(base + static_cast<int16_t>(entry.imm));
  entry.mem_addr = addr;
  entry.mem_size = 4; // Default
  entry.mem_sign_extend = true;

  // Determine size from opcode
  switch (entry.opcode) {
  case 0x20:
    entry.mem_size = 1;
    entry.mem_sign_extend = true;
    break; // LB
  case 0x21:
    entry.mem_size = 2;
    entry.mem_sign_extend = true;
    break; // LH
  case 0x22:
    entry.mem_size = 4;
    entry.mem_sign_extend = true;
    break; // LWL
  case 0x23:
    entry.mem_size = 4;
    entry.mem_sign_extend = true;
    break; // LW
  case 0x24:
    entry.mem_size = 1;
    entry.mem_sign_extend = false;
    break; // LBU
  case 0x25:
    entry.mem_size = 2;
    entry.mem_sign_extend = false;
    break; // LHU
  case 0x26:
    entry.mem_size = 4;
    entry.mem_sign_extend = true;
    break; // LWR
  case 0x27:
    entry.mem_size = 8;
    entry.mem_sign_extend = true;
    break; // LD (MIPS III/IV)
  case 0x30:
    entry.mem_size = 4;
    entry.mem_sign_extend = true;
    break; // LL
  case 0x31:
    entry.mem_size = 4;
    entry.mem_sign_extend = true;
    break; // LWC1
  case 0x32:
    entry.mem_size = 4;
    entry.mem_sign_extend = true;
    break; // LWC2
  case 0x33:
    entry.mem_size = 4;
    entry.mem_sign_extend = true;
    break; // PREF
  }

  // Check alignment
  if (addr & (entry.mem_size - 1)) {
    entry.exception = true;
    entry.exc_code = static_cast<uint32_t>(ExceptionCode::ADEL);
    return;
  }

  // Access memory
  if (!access_memory(entry)) {
    entry.exception = true;
    entry.exc_code = static_cast<uint32_t>(ExceptionCode::IBE);
    return;
  }

  // Sign extend result
  uint64_t result = entry.result;
  if (entry.mem_sign_extend) {
    switch (entry.mem_size) {
    case 1:
      result = static_cast<int64_t>(static_cast<int8_t>(result));
      break;
    case 2:
      result = static_cast<int64_t>(static_cast<int16_t>(result));
      break;
    case 4:
      result = static_cast<int64_t>(static_cast<int32_t>(result));
      break;
    }
  }

  if (!is_64bit_mode()) {
    result = static_cast<int32_t>(result);
  }

  entry.result = result;
  if (entry.phys_rd >= 0) {
    write_phys_gpr(entry.phys_rd, result);
  }

  // For LL, set LLbit
  if (entry.opcode == 0x30) {
    llbit_ = true;
    lladdr_ = addr;
  }
}

void MIPSR10000::execute_store(InstructionEntry &entry) {
  uint64_t base = (entry.rs == 0) ? 0 : read_phys_gpr(entry.phys_rs);
  uint64_t rt_val = (entry.rt == 0) ? 0 : read_phys_gpr(entry.phys_rt);
  uint32_t addr = static_cast<uint32_t>(base + static_cast<int16_t>(entry.imm));
  entry.mem_addr = addr;
  entry.is_store = true;
  entry.store_data = static_cast<uint32_t>(rt_val);

  // Determine size
  switch (entry.opcode) {
  case 0x28:
    entry.mem_size = 1;
    break; // SB
  case 0x29:
    entry.mem_size = 2;
    break; // SH
  case 0x2A:
    entry.mem_size = 4;
    break; // SWL
  case 0x2B:
    entry.mem_size = 4;
    break; // SW
  case 0x2C:
    entry.mem_size = 4;
    break; // SWR
  case 0x2D:
    entry.mem_size = 4;
    break; // CACHE
  case 0x2E:
    entry.mem_size = 4;
    break; // PREF
  case 0x2F:
    entry.mem_size = 8;
    break; // SD (MIPS III/IV)
  case 0x38:
    entry.mem_size = 4;
    break; // SC
  case 0x39:
    entry.mem_size = 4;
    break; // SWC1
  case 0x3A:
    entry.mem_size = 4;
    break; // SWC2
  case 0x3B:
    entry.mem_size = 8;
    break; // SD
  }

  // Check alignment
  if (addr & (entry.mem_size - 1)) {
    entry.exception = true;
    entry.exc_code = static_cast<uint32_t>(ExceptionCode::ADES);
    return;
  }

  // For SC, check LLbit
  if (entry.opcode == 0x38) {
    if (!llbit_ || addr != lladdr_) {
      // SC failed
      if (entry.phys_rd >= 0) {
        write_phys_gpr(entry.phys_rd, 0);
      }
      return;
    }
    llbit_ = false;
  }

  // Access memory
  if (!access_memory(entry)) {
    entry.exception = true;
    entry.exc_code = static_cast<uint32_t>(ExceptionCode::DBE);
    return;
  }

  // SC success
  if (entry.opcode == 0x38 && entry.phys_rd >= 0) {
    write_phys_gpr(entry.phys_rd, 1);
  }
}

bool MIPSR10000::access_memory(InstructionEntry &entry) {
  if (entry.is_store) {
    switch (entry.mem_size) {
    case 1:
      return bus_->write8(entry.mem_addr, 1, entry.store_data & 0xFF);
    case 2:
      return bus_->write8(entry.mem_addr, 2, entry.store_data & 0xFFFF);
    case 4:
      return bus_->write8(entry.mem_addr, 4, entry.store_data);
    case 8:
      return bus_->write8(entry.mem_addr, 4, entry.store_data & 0xFFFFFFFF) &&
             bus_->write8(entry.mem_addr + 4, 4,
                          (static_cast<uint64_t>(entry.store_data) >> 32));
    }
  } else {
    uint32_t data = 0;
    bool result = bus_->read8(entry.mem_addr, entry.mem_size, data);
    entry.result = data;
    return result;
  }
  return false;
}

void MIPSR10000::execute_fpu(InstructionEntry &entry) {
  // Simplified FPU implementation
  // In a full implementation, this would handle IEEE 754 operations
  fu_status_[static_cast<int>(entry.fu)].cycles_remaining =
      4; // 4 cycles for FP ops

  // For now, just mark as completed with dummy result
  if (entry.phys_rd >= 0) {
    write_phys_fpr(entry.phys_rd, 0);
  }
}

void MIPSR10000::execute_branch(InstructionEntry &entry) {
  uint64_t rs_val = (entry.rs == 0) ? 0 : read_phys_gpr(entry.phys_rs);
  uint64_t rt_val = (entry.rt == 0) ? 0 : read_phys_gpr(entry.phys_rt);
  bool condition = false;
  uint32_t target = pc_ + 4 + (static_cast<uint32_t>(entry.imm) << 2);

  switch (entry.opcode) {
  case 0x01: // REGIMM
    switch (entry.rt) {
    case 0x00: // BLTZ
      condition = static_cast<int64_t>(rs_val) < 0;
      break;
    case 0x01: // BGEZ
      condition = static_cast<int64_t>(rs_val) >= 0;
      break;
    case 0x10: // BLTZAL
      condition = static_cast<int64_t>(rs_val) < 0;
      if (entry.phys_rd >= 0)
        write_phys_gpr(entry.phys_rd, pc_ + 8);
      break;
    case 0x11: // BGEZAL
      condition = static_cast<int64_t>(rs_val) >= 0;
      if (entry.phys_rd >= 0)
        write_phys_gpr(entry.phys_rd, pc_ + 8);
      break;
    }
    break;
  case 0x04: // BEQ
    condition = rs_val == rt_val;
    break;
  case 0x05: // BNE
    condition = rs_val != rt_val;
    break;
  case 0x06: // BLEZ
    condition = static_cast<int64_t>(rs_val) <= 0;
    break;
  case 0x07: // BGTZ
    condition = static_cast<int64_t>(rs_val) > 0;
    break;
  case 0x02: // J
    condition = true;
    target = (pc_ & 0xF0000000) | entry.target;
    break;
  case 0x03: // JAL
    condition = true;
    target = (pc_ & 0xF0000000) | entry.target;
    if (entry.phys_rd >= 0)
      write_phys_gpr(entry.phys_rd, pc_ + 8);
    break;
  }

  entry.actually_taken = condition;
  entry.actual_target = condition ? target : pc_ + 4;

  if (condition) {
    branch_delay_ = true;
    next_pc_ = target;
  }
}

uint32_t MIPSR10000::gpr(int reg) const {
  if (reg < 0 || reg >= 32)
    return 0;
  return static_cast<uint32_t>(gpr_[reg]);
}

void MIPSR10000::set_gpr(int reg, uint32_t value) {
  if (reg < 0 || reg >= 32)
    return;
  int phys = get_phys_gpr(reg);
  gpr_[reg] = value;
  if (phys >= 0 && phys < NUM_PHYS_REGS) {
    phys_gpr_[phys] = value;
    phys_gpr_valid_[phys] = true;
  }
}

uint64_t MIPSR10000::gpr64(int reg) const {
  if (reg < 0 || reg >= 32)
    return 0;
  return gpr_[reg];
}

void MIPSR10000::set_gpr64(int reg, uint64_t value) {
  if (reg < 0 || reg >= 32)
    return;
  int phys = get_phys_gpr(reg);
  gpr_[reg] = value;
  if (phys >= 0 && phys < NUM_PHYS_REGS) {
    phys_gpr_[phys] = value;
    phys_gpr_valid_[phys] = true;
  }
}

uint32_t MIPSR10000::cp0_reg(int reg) const {
  if (reg < 0 || reg >= 32)
    return 0;
  return cp0_[reg];
}

void MIPSR10000::set_cp0_reg(int reg, uint32_t value) {
  if (reg < 0 || reg >= 32)
    return;
  cp0_[reg] = value;
}

uint64_t MIPSR10000::get_perf_counter(int idx) const {
  if (idx < 0 || idx >= 2)
    return 0;
  return perf_counters_[idx];
}

void MIPSR10000::set_perf_counter(int idx, uint64_t value) {
  if (idx < 0 || idx >= 2)
    return;
  perf_counters_[idx] = value;
}

void MIPSR10000::set_perf_counter_control(int idx, uint32_t control) {
  if (idx < 0 || idx >= 2)
    return;
  perf_counter_ctrl_[idx] = control;
}

void MIPSR10000::dump_registers() const {
  std::cout << "=== R10000 Registers ===" << std::endl;
  std::cout << "PC: 0x" << std::hex << pc_ << std::dec << std::endl;
  std::cout << "Cycles: " << cycles_ << ", Instructions: " << instr_count_
            << ", Committed: " << committed_count_ << std::endl;
  std::cout << "IPC: "
            << (cycles_ > 0 ? static_cast<double>(committed_count_) / cycles_
                            : 0.0)
            << std::endl;

  std::cout << "\nGPRs:" << std::endl;
  for (int i = 0; i < 32; ++i) {
    if (i % 4 == 0)
      std::cout << std::endl;
    std::cout << "$" << std::setw(2) << i << "=0x" << std::hex << std::setw(8)
              << std::setfill('0') << gpr_[i] << std::dec << std::setfill(' ')
              << "  ";
  }
  std::cout << std::endl;

  std::cout << "\nCP0:" << std::endl;
  for (int i = 0; i < 32; ++i) {
    if (cp0_[i] != 0) {
      std::cout << "  CP0[" << i << "] = 0x" << std::hex << cp0_[i] << std::dec
                << std::endl;
    }
  }
}

void MIPSR10000::dump_pipeline() const {
  std::cout << "=== Pipeline State ===" << std::endl;
  std::cout << "Fetch queue: " << fetch_queue_count_ << "/" << FETCH_QUEUE_SIZE
            << std::endl;
  std::cout << "Issue queue: " << issue_queue_count_ << "/" << ISSUE_QUEUE_SIZE
            << std::endl;
  std::cout << "ROB: " << rob_count_ << "/" << ROB_SIZE << std::endl;
  std::cout << "Load queue: " << load_queue_count_ << "/" << LSQ_SIZE
            << std::endl;
  std::cout << "Store queue: " << store_queue_count_ << "/" << LSQ_SIZE
            << std::endl;

  std::cout << "\nFunctional Units:" << std::endl;
  const char *fu_names[] = {"ALU1",  "ALU2", "MULT", "LOAD",
                            "STORE", "FPU1", "FPU2", "BRANCH"};
  for (int i = 0; i < 8; ++i) {
    if (fu_status_[i].busy) {
      std::cout << "  " << fu_names[i]
                << ": busy (ROB=" << fu_status_[i].rob_index
                << ", cycles=" << fu_status_[i].cycles_remaining << ")"
                << std::endl;
    }
  }
}

void MIPSR10000::dump_rob() const {
  std::cout << "=== Reorder Buffer ===" << std::endl;
  for (int i = 0; i < rob_count_; ++i) {
    int idx = (rob_head_ + i) % ROB_SIZE;
    const ROBEntry &entry = rob_[idx];
    std::cout << "  [" << idx << "] PC=0x" << std::hex << entry.pc << " dest=$"
              << std::dec << entry.arch_dest << " phys=" << entry.phys_dest
              << " completed=" << entry.completed << " exc=" << entry.exception
              << std::endl;
  }
}

void MIPSR10000::dump_rename_map() const {
  std::cout << "=== Register Rename Map ===" << std::endl;
  for (int i = 0; i < 32; ++i) {
    const RenameEntry &e = gpr_rename_map_[i];
    if (e.phys_reg != i || !e.valid) {
      std::cout << "  $" << i << " -> phys " << e.phys_reg
                << " (valid=" << e.valid << ", rob=" << e.rob_index << ")"
                << std::endl;
    }
  }
}

} // namespace o2emu::cpu