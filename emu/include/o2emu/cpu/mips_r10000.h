/**
 * @file mips_r10000.h
 * @brief MIPS R10000/R12000 CPU core interface
 *
 * The R10000 (T5) is a superscalar MIPS IV implementation with:
 * - 4-issue superscalar pipeline
 * - Out-of-order execution with register renaming
 * - 64-bit FPU with fused multiply-add
 * - 32KB L1 I-cache, 32KB L1 D-cache
 * - External L2 cache (1-16MB)
 * - Branch prediction with 2-bit counters
 * - 64-entry instruction TLB, 64-entry data TLB
 * - 64-bit system bus at 100-200 MHz
 *
 * R12000 adds:
 * - Higher clock rates (300-400 MHz)
 * - Improved branch prediction
 * - Larger L2 cache support
 * - Prefetch instructions
 */

#pragma once

#include <array>
#include <o2emu/cpu/mips_r5000.h>
#include <o2emu/system/bus.h>
#include <vector>

namespace o2emu::cpu {

// R10000/R12000 specific CP0 registers
enum class R10K_CP0_Register : uint32_t {
  // Standard MIPS IV CP0 registers (same as R5000)
  INDEX = 0,
  RANDOM = 1,
  ENTRYLO0 = 2,
  ENTRYLO1 = 3,
  CONTEXT = 4,
  PAGEMASK = 5,
  WIRED = 6,
  BADVADDR = 8,
  COUNT = 9,
  ENTRYHI = 10,
  COMPARE = 11,
  STATUS = 12,
  CAUSE = 13,
  EPC = 14,
  PRID = 15,
  CONFIG = 16,
  LLADDR = 17,
  WATCHLO = 18,
  WATCHHI = 19,
  XCONTEXT = 20,
  ECC = 26,
  CACHEERR = 27,
  TAGLO = 28,
  TAGHI = 29,
  ERROREPC = 30,

  // R10000-specific registers
  PERFCNT = 25,      // Performance counter
  PERFCNT_CTRL = 24, // Performance counter control

  // R12000-specific
  PREFETCH_CTRL = 31, // Prefetch control
};

// R10000 PRID values
constexpr uint32_t PRID_R10000 = 0x00002000;  // R10000
constexpr uint32_t PRID_R10000A = 0x00002100; // R10000A
constexpr uint32_t PRID_R12000 = 0x00002200;  // R12000
constexpr uint32_t PRID_R12000A = 0x00002300; // R12000A
constexpr uint32_t PRID_R14000 = 0x00002400;  // R14000 (never released)

// R10000 Config register bits
constexpr uint32_t CONFIG_K0 = 0x00000007;  // Kseg0 coherency algorithm
constexpr uint32_t CONFIG_CU = 0x00000008;  // Coprocessor usable
constexpr uint32_t CONFIG_BE = 0x00000080;  // Big endian
constexpr uint32_t CONFIG_AT = 0x00000100;  // Architecture type (MIPS IV)
constexpr uint32_t CONFIG_AR = 0x00000E00;  // Architecture revision
constexpr uint32_t CONFIG_MT = 0x00007000;  // MMU type
constexpr uint32_t CONFIG_VI = 0x00010000;  // Virtually indexed cache
constexpr uint32_t CONFIG_K23 = 0x00060000; // Kseg2/3 coherency
constexpr uint32_t CONFIG_EE = 0x00100000;  // Exception extension
constexpr uint32_t CONFIG_IE = 0x00200000;  // Interrupt extension
constexpr uint32_t CONFIG_SB = 0x00400000;  // Simple bus
constexpr uint32_t CONFIG_SS = 0x00800000;  // Split secondary cache

// R10000 Status register additional bits
constexpr uint32_t STATUS_FR = 0x04000000; // 64-bit FPU enable
constexpr uint32_t STATUS_RP = 0x08000000; // Reduced power mode

// R10000 Cause register additional bits
constexpr uint32_t CAUSE_WP = 0x00800000;  // Watchpoint
constexpr uint32_t CAUSE_PCI = 0x08000000; // Performance counter interrupt

// Pipeline stages for superscalar simulation
enum class PipelineStage : uint8_t {
  IF1 = 0,   // Instruction fetch 1
  IF2 = 1,   // Instruction fetch 2
  ID = 2,    // Instruction decode
  IS = 3,    // Issue (register rename, dispatch)
  EX = 4,    // Execute
  WB = 5,    // Writeback
  COMMIT = 6 // Commit (in-order retirement)
};

// Functional units
enum class FunctionalUnit : uint8_t {
  ALU1 = 0,  // Integer ALU 1
  ALU2 = 1,  // Integer ALU 2
  MULT = 2,  // Multiply/Divide
  LOAD = 3,  // Load unit
  STORE = 4, // Store unit
  FPU1 = 5,  // FPU add/sub/convert
  FPU2 = 6,  // FPU multiply/divide
  BRANCH = 7 // Branch unit
};

// Instruction queue entry for out-of-order execution
struct InstructionEntry {
  uint32_t pc = 0;
  uint32_t instr = 0;
  uint32_t opcode = 0;
  uint32_t funct = 0;
  uint8_t rs = 0, rt = 0, rd = 0;
  int16_t imm = 0;
  uint32_t target = 0;

  // Register renaming
  int phys_rs = -1, phys_rt = -1, phys_rd = -1;
  int old_phys_rd = -1;

  // Dependencies
  int dep_rs = -1, dep_rt = -1;
  bool rs_ready = false, rt_ready = false;

  // Execution
  FunctionalUnit fu = FunctionalUnit::ALU1;
  PipelineStage stage = PipelineStage::IF1;
  int cycles_remaining = 0;
  uint64_t result = 0;
  bool exception = false;
  uint32_t exc_code = 0;
  bool in_delay_slot = false;
  bool predicted_taken = false;
  uint32_t predicted_target = 0;

  // Memory access
  uint32_t mem_addr = 0;
  uint32_t mem_size = 0;
  bool mem_sign_extend = false;
  bool is_store = false;
  uint32_t store_data = 0;
  uint8_t shamt = 0;
  bool actually_taken = false;
  uint32_t actual_target = 0;
};

// Reorder buffer entry
struct ROBEntry {
  uint32_t pc = 0;
  uint32_t instr = 0;
  int phys_dest = -1;
  int arch_dest = -1;
  uint64_t result = 0;
  bool completed = false;
  bool exception = false;
  uint32_t exc_code = 0;
  uint32_t bad_addr = 0;
  bool is_branch = false;
  bool predicted_taken = false;
  uint32_t predicted_target = 0;
  bool actually_taken = false;
  uint32_t actual_target = 0;
  bool in_delay_slot = false;
};

// Register rename map entry
struct RenameEntry {
  int phys_reg = -1;
  bool valid = false;
  int rob_index = -1;
};

// Branch predictor entry (2-bit saturating counter)
struct BranchPredictorEntry {
  uint32_t pc = 0;
  uint32_t target = 0;
  uint8_t counter = 2; // 0=strongly not taken, 3=strongly taken
  bool valid = false;
};

class MIPSR10000 {
public:
  enum class Variant { R10000, R10000A, R12000, R12000A };

  explicit MIPSR10000(system::Bus *bus, Variant variant = Variant::R10000);
  ~MIPSR10000() = default;

  // Non-copyable, movable
  MIPSR10000(const MIPSR10000 &) = delete;
  MIPSR10000 &operator=(const MIPSR10000 &) = delete;
  MIPSR10000(MIPSR10000 &&) = default;
  MIPSR10000 &operator=(MIPSR10000 &&) = default;

  // Initialize CPU state
  void reset();

  // Execute for N cycles
  void tick(uint64_t cycles);

  // State access
  uint32_t gpr(int reg) const;
  void set_gpr(int reg, uint32_t value);

  uint64_t gpr64(int reg) const;
  void set_gpr64(int reg, uint64_t value);

  uint32_t cp0_reg(int reg) const;
  void set_cp0_reg(int reg, uint32_t value);

  uint32_t pc() const { return pc_; }
  void set_pc(uint32_t pc) { pc_ = pc; }

  uint64_t cycles() const { return cycles_; }
  uint64_t instructions() const { return instr_count_; }
  uint64_t committed_instructions() const { return committed_count_; }

  // Performance counters
  uint64_t get_perf_counter(int idx) const;
  void set_perf_counter(int idx, uint64_t value);
  void set_perf_counter_control(int idx, uint32_t control);

  // Debugging
  void dump_registers() const;
  void dump_pipeline() const;
  void dump_rob() const;
  void dump_rename_map() const;

  // Pipeline statistics
  struct PipelineStats {
    uint64_t cycles = 0;
    uint64_t instructions_fetched = 0;
    uint64_t instructions_issued = 0;
    uint64_t instructions_committed = 0;
    uint64_t branches_predicted = 0;
    uint64_t branches_mispredicted = 0;
    uint64_t load_store_issued = 0;
    uint64_t fp_issued = 0;
    uint64_t stalls_structural = 0;
    uint64_t stalls_data = 0;
    uint64_t stalls_control = 0;
  };
  PipelineStats get_stats() const { return stats_; }

  // Check if CPU is in 64-bit mode
  bool is_64bit_mode() const {
    return (cp0_[static_cast<int>(R10K_CP0_Register::STATUS)] & 0x00000080) !=
           0; // KX bit
  }

  // Check if FPU is 64-bit mode
  bool is_fpu_64bit() const {
    return (cp0_[static_cast<int>(R10K_CP0_Register::STATUS)] & STATUS_FR) != 0;
  }

private:
  system::Bus *bus_;
  Variant variant_;

  // Architectural state (32 architectural registers)
  std::array<uint64_t, 32> gpr_; // 64-bit GPRs
  std::array<uint64_t, 32> fpr_; // 64-bit FPRs
  std::array<uint32_t, 32> cp0_; // CP0 registers
  uint64_t hi_ = 0, lo_ = 0;     // Multiply/divide
  uint32_t pc_ = 0;
  uint32_t fcr0_ = 0;
  uint32_t fcr31_ = 0;
  bool llbit_ = false;
  uint32_t lladdr_ = 0;

  // Physical register file (128 registers for renaming)
  static constexpr int NUM_PHYS_REGS = 128;
  std::array<uint64_t, NUM_PHYS_REGS> phys_gpr_;
  std::array<uint64_t, NUM_PHYS_REGS> phys_fpr_;
  std::array<bool, NUM_PHYS_REGS> phys_gpr_valid_;
  std::array<bool, NUM_PHYS_REGS> phys_fpr_valid_;
  std::array<int, NUM_PHYS_REGS>
      phys_gpr_arch_map_; // Which architectural reg maps to this phys reg
  std::array<int, NUM_PHYS_REGS> phys_fpr_arch_map_;
  int free_phys_gpr_head_ = 32; // First 32 are architectural
  int free_phys_fpr_head_ = 32;

  // Register rename map (architectural -> physical)
  std::array<RenameEntry, 32> gpr_rename_map_;
  std::array<RenameEntry, 32> fpr_rename_map_;

  // Instruction fetch queue
  static constexpr int FETCH_QUEUE_SIZE = 16;
  std::array<InstructionEntry, FETCH_QUEUE_SIZE> fetch_queue_;
  int fetch_queue_head_ = 0, fetch_queue_tail_ = 0, fetch_queue_count_ = 0;

  // Issue queue (reservation stations)
  static constexpr int ISSUE_QUEUE_SIZE = 32;
  std::array<InstructionEntry, ISSUE_QUEUE_SIZE> issue_queue_;
  int issue_queue_count_ = 0;

  // Reorder buffer (64 entries)
  static constexpr int ROB_SIZE = 64;
  std::array<ROBEntry, ROB_SIZE> rob_;
  int rob_head_ = 0, rob_tail_ = 0, rob_count_ = 0;

  // Branch predictor (512 entries, direct-mapped)
  static constexpr int BP_SIZE = 512;
  std::array<BranchPredictorEntry, BP_SIZE> branch_predictor_;

  // Functional unit status
  struct FUStatus {
    bool busy = false;
    int rob_index = -1;
    int cycles_remaining = 0;
  };
  std::array<FUStatus, 8> fu_status_;

  // Load/store queue
  static constexpr int LSQ_SIZE = 16;
  std::array<InstructionEntry, LSQ_SIZE> load_queue_;
  std::array<InstructionEntry, LSQ_SIZE> store_queue_;
  int load_queue_count_ = 0, store_queue_count_ = 0;

  // Performance counters
  std::array<uint64_t, 2> perf_counters_ = {0, 0};
  std::array<uint32_t, 2> perf_counter_ctrl_ = {0, 0};

  // Statistics
  PipelineStats stats_;
  uint64_t cycles_ = 0;
  uint64_t instr_count_ = 0;
  uint64_t committed_count_ = 0;

  // Pipeline control
  bool branch_delay_ = false;
  uint32_t next_pc_ = 0;
  bool flush_pipeline_ = false;
  uint32_t flush_target_ = 0;

  // Internal methods
  void fetch_stage();
  void decode_stage();
  void issue_stage();
  void execute_stage();
  void writeback_stage();
  void commit_stage();

  // Instruction decode
  InstructionEntry decode_instruction(uint32_t pc, uint32_t instr);

  // Register renaming
  void rename_registers(InstructionEntry &entry);
  int allocate_phys_gpr(int arch_reg);
  int allocate_phys_fpr(int arch_reg);
  void free_phys_gpr(int phys_reg);
  void free_phys_fpr(int phys_reg);

  // Branch prediction
  bool predict_branch(uint32_t pc, uint32_t &predicted_target);
  void update_branch_predictor(uint32_t pc, bool taken, uint32_t actual_target);

  // Functional unit allocation
  bool allocate_fu(InstructionEntry &entry);
  void free_fu(FunctionalUnit fu);

  // Execution
  void execute_instruction(InstructionEntry &entry);
  void execute_alu(InstructionEntry &entry);
  void execute_mult_div(InstructionEntry &entry);
  void execute_load(InstructionEntry &entry);
  void execute_store(InstructionEntry &entry);
  void execute_fpu(InstructionEntry &entry);
  void execute_branch(InstructionEntry &entry);

  // Memory access
  bool access_memory(InstructionEntry &entry);

  // Exception handling
  void handle_exception(uint32_t exc_code, uint32_t bad_addr);
  void check_interrupts();

  // Commit
  void commit_instruction(ROBEntry &entry);
  void retire_instruction();

  // Pipeline flush
  void flush_pipeline(uint32_t target_pc);

  // Helper: get physical register for architectural register
  int get_phys_gpr(int arch_reg) const;
  int get_phys_fpr(int arch_reg) const;

  // Helper: read/write physical register
  uint64_t read_phys_gpr(int phys_reg) const;
  void write_phys_gpr(int phys_reg, uint64_t value);
  uint64_t read_phys_fpr(int phys_reg) const;
  void write_phys_fpr(int phys_reg, uint64_t value);

  // CP0 register access
  uint32_t read_cp0(int reg) const;
  void write_cp0(int reg, uint32_t value);

  // Initialize free physical register lists
  void init_free_lists();

  // R10000-specific instructions
  void execute_prefetch(InstructionEntry &entry);
  void execute_conditional_move(InstructionEntry &entry);
  void execute_mips_iv(InstructionEntry &entry);
};

} // namespace o2emu::cpu