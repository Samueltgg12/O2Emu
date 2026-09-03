#include "o2emu/cpu_interface.h"
#include "o2emu/bus.h"
#include "o2emu/logging.h"
#include <cstring>
#include <unordered_set>

namespace o2emu {
namespace {

constexpr uint32_t kMipsStatusIE = 0x00000001u;
constexpr uint32_t kMipsStatusEXL = 0x00000002u;
constexpr uint32_t kMipsStatusERL = 0x00000004u;
constexpr uint32_t kMipsStatusBE = 0x00400000u;
constexpr uint32_t kMipsCauseIPShift = 8u;
constexpr uint32_t kMipsCauseExcCodeMask = 0x7cu;
constexpr uint32_t kMipsCauseExcCodeShift = 2u;
constexpr uint32_t kMipsCauseBD = 0x80000000u;
constexpr uint32_t kExceptionVectorGeneral = 0x80000180u;
constexpr uint32_t kExceptionVectorReset = 0xbfc00000u;

inline uint32_t sign_extend16(uint32_t value) {
  const uint32_t sign = value & 0x00008000u;
  return sign ? (value | 0xffff0000u) : value;
}

inline bool irq_masked(const CpuState &state, uint32_t irq) {
  if (irq >= 8u) {
    return true;
  }
  const uint32_t mask = 1u << (irq + 8u);
  return (state.cp0.status & mask) == 0u;
}

} // namespace

// ---------------------------------------------------------------------------
// Interpreter CPU core (MIPS R5000 / R10000 / R12000 family)
//
// This core intentionally starts as a precise, compact interpreter for the
// instruction subset needed by the PROM and early OS code. It models the core
// CPU state, the CP0 system coprocessor, and the standard interrupt/exception
// flow used by SGI IP32 hardware.
// ---------------------------------------------------------------------------

class InterpreterCpu : public CpuInterface {
public:
  explicit InterpreterCpu(CpuModel model = CpuModel::R5000) : model_(model) {}

  void init(SystemBus *bus) override { bus_ = bus; }

  void reset(uint32_t reset_vector = 0xbfc00000) override {
    state_ = CpuState{};
    state_.model = model_;
    state_.pc = reset_vector;
    state_.next_pc = reset_vector + 4;

    switch (model_) {
    case CpuModel::R5000:
      state_.cp0.prid = 0x00002300u;
      break;
    case CpuModel::R10000:
      state_.cp0.prid = 0x00002400u;
      break;
    case CpuModel::R12000:
      state_.cp0.prid = 0x00002500u;
      break;
    }

    state_.cp0.config = 0x00000000u;
    state_.cp0.status = kMipsStatusBE;
    state_.cp0.cause = 0x00000000u;
    running_ = false;
    O2E_INFO(LogCategory::CPU, "CPU reset (model={}, PC=0x{:08x})",
             model_name(model_), reset_vector);
  }

  void step() override {
    if (state_.exception_pending) {
      handle_exception();
      return;
    }

    if (bus_ == nullptr) {
      O2E_WARN(LogCategory::CPU, "step() with no bus attached; halting");
      running_ = false;
      return;
    }

    if (has_breakpoint(state_.pc)) {
      O2E_INFO(LogCategory::CPU, "Breakpoint hit at 0x{:08x}", state_.pc);
      state_.exception_pending = true;
      state_.exception_code = 0x0du; // Break
      state_.exception_badvaddr = state_.pc;
      handle_exception();
      return;
    }

    state_.cycles += 1;
    const uint32_t pc = state_.pc;
    const uint32_t instr = read_physical(pc);
    state_.instructions += 1;
    state_.next_pc = pc + 4u;
    state_.in_delay_slot = false;
    state_.branch_taken = false;
    state_.branch_target = 0;

    execute_instruction(instr, pc);

    if (!state_.exception_pending) {
      state_.pc = state_.next_pc;
    }
  }

  void run(uint64_t max_instructions = UINT64_MAX) override {
    running_ = true;
    uint64_t executed = 0;
    while (running_ && executed < max_instructions) {
      if (state_.exception_pending) {
        handle_exception();
      }
      if (!running_) {
        break;
      }
      step();
      ++executed;
    }
  }

  void stop() override { running_ = false; }

  const CpuState &state() const override { return state_; }
  CpuState &mutable_state() override { return state_; }

  void raise_interrupt(uint32_t irq) override {
    if (irq >= 8u) {
      return;
    }

    const uint32_t bit = 1u << (kMipsCauseIPShift + irq);
    state_.cp0.cause |= bit;
    if (state_.cp0.status & kMipsStatusIE) {
      if ((state_.cp0.status & kMipsStatusEXL) == 0u &&
          (state_.cp0.status & kMipsStatusERL) == 0u) {
        state_.exception_pending = true;
        state_.exception_code = 0u;
        state_.exception_badvaddr = state_.pc;
      }
    }
  }

  void lower_interrupt(uint32_t irq) override {
    if (irq >= 8u) {
      return;
    }
    state_.cp0.cause &= ~(1u << (kMipsCauseIPShift + irq));
  }

  uint32_t read_physical(uint32_t addr) override {
    if (bus_) {
      return bus_->read32(addr);
    }
    return 0;
  }

  void write_physical(uint32_t addr, uint32_t val) override {
    if (bus_) {
      bus_->write32(addr, val);
    }
  }

  void add_breakpoint(uint32_t addr) override { breakpoints_.insert(addr); }
  void remove_breakpoint(uint32_t addr) override { breakpoints_.erase(addr); }
  bool has_breakpoint(uint32_t addr) const override {
    return breakpoints_.count(addr) > 0;
  }

  void add_watchpoint(uint32_t addr, uint32_t size, bool read,
                      bool write) override {
    (void)addr;
    (void)size;
    (void)read;
    (void)write;
  }

  void remove_watchpoint(uint32_t addr) override { (void)addr; }

  void add_tlb_entry(const TlbEntry &entry) override {
    for (auto &current : state_.tlb) {
      if (!current.valid) {
        current = entry;
        return;
      }
    }
    state_.tlb[0] = entry;
  }

  void remove_tlb_entry(uint32_t vpn) override {
    for (auto &entry : state_.tlb) {
      if (entry.valid && entry.vpn == vpn) {
        entry = TlbEntry{};
        return;
      }
    }
  }

  uint32_t translate_address(uint32_t vaddr) const override {
    for (const auto &entry : state_.tlb) {
      if (!entry.valid) {
        continue;
      }
      const uint32_t page_mask = entry.page_mask ? entry.page_mask : 0xfffu;
      const uint32_t page = vaddr & ~page_mask;
      if (page == entry.vpn) {
        return (vaddr & page_mask) | (entry.pfn & ~page_mask);
      }
    }
    return vaddr;
  }

  void set_cache_mode(bool icache_enabled, bool dcache_enabled) override {
    state_.cache.icache_enabled = icache_enabled;
    state_.cache.dcache_enabled = dcache_enabled;
  }

  std::string disassemble(uint32_t pc, uint32_t instr) const override {
    (void)pc;
    const uint32_t op = instr >> 26;
    const uint32_t funct = instr & 0x3fu;

    switch (op) {
    case 0x00:
      switch (funct) {
      case 0x00:
        return "sll";
      case 0x02:
        return "srl";
      case 0x03:
        return "sra";
      case 0x04:
        return "sllv";
      case 0x06:
        return "srlv";
      case 0x07:
        return "srav";
      case 0x08:
        return "jr";
      case 0x09:
        return "jalr";
      case 0x0c:
        return "syscall";
      case 0x20:
        return "add";
      case 0x21:
        return "addu";
      case 0x22:
        return "sub";
      case 0x23:
        return "subu";
      case 0x24:
        return "and";
      case 0x25:
        return "or";
      case 0x26:
        return "xor";
      case 0x27:
        return "nor";
      case 0x2a:
        return "slt";
      case 0x2b:
        return "sltu";
      default:
        break;
      }
      break;
    case 0x02:
      return "j";
    case 0x03:
      return "jal";
    case 0x04:
      return "beq";
    case 0x05:
      return "bne";
    case 0x09:
      return "addiu";
    case 0x0d:
      return "ori";
    case 0x0f:
      return "lui";
    case 0x23:
      return "lw";
    case 0x2b:
      return "sw";
    default:
      break;
    }

    return "unk";
  }

private:
  void execute_instruction(uint32_t instr, uint32_t pc) {
    const uint32_t op = instr >> 26;
    const uint32_t rs = (instr >> 21) & 0x1fu;
    const uint32_t rt = (instr >> 16) & 0x1fu;
    const uint32_t rd = (instr >> 11) & 0x1fu;
    const uint32_t shamt = (instr >> 6) & 0x1fu;
    const uint32_t funct = instr & 0x3fu;
    const uint32_t imm = instr & 0xffffu;
    const uint32_t target = instr & 0x03ffffffu;

    switch (op) {
    case 0x00:
      switch (funct) {
      case 0x00:
        state_.gpr[rd] = state_.gpr[rt] << shamt;
        break;
      case 0x02:
        state_.gpr[rd] = state_.gpr[rt] >> shamt;
        break;
      case 0x03:
        state_.gpr[rd] = static_cast<uint32_t>(
            static_cast<int32_t>(state_.gpr[rt]) >> shamt);
        break;
      case 0x08:
        state_.next_pc = state_.gpr[rs];
        break;
      case 0x09:
        state_.gpr[rd] = state_.next_pc;
        state_.next_pc = state_.gpr[rs];
        break;
      case 0x0c:
        state_.exception_pending = true;
        state_.exception_code = 0x08u;
        state_.exception_badvaddr = pc;
        break;
      case 0x20:
        state_.gpr[rd] =
            static_cast<uint32_t>(static_cast<int32_t>(state_.gpr[rs]) +
                                  static_cast<int32_t>(state_.gpr[rt]));
        break;
      case 0x21:
        state_.gpr[rd] = state_.gpr[rs] + state_.gpr[rt];
        break;
      case 0x24:
        state_.gpr[rd] = state_.gpr[rs] & state_.gpr[rt];
        break;
      case 0x25:
        state_.gpr[rd] = state_.gpr[rs] | state_.gpr[rt];
        break;
      case 0x26:
        state_.gpr[rd] = state_.gpr[rs] ^ state_.gpr[rt];
        break;
      case 0x27:
        state_.gpr[rd] = ~(state_.gpr[rs] | state_.gpr[rt]);
        break;
      case 0x2a:
        state_.gpr[rd] = (static_cast<int32_t>(state_.gpr[rs]) <
                          static_cast<int32_t>(state_.gpr[rt]))
                             ? 1u
                             : 0u;
        break;
      case 0x2b:
        state_.gpr[rd] = (state_.gpr[rs] < state_.gpr[rt]) ? 1u : 0u;
        break;
      default:
        O2E_WARN(LogCategory::CPU, "Unsupported opcode 0x{:08x} at 0x{:08x}",
                 instr, pc);
        break;
      }
      break;

    case 0x02:
      state_.next_pc = ((pc + 4u) & 0xf0000000u) | (target << 2u);
      state_.branch_taken = true;
      state_.branch_target = state_.next_pc;
      break;
    case 0x03:
      state_.gpr[31] = pc + 8u;
      state_.next_pc = ((pc + 4u) & 0xf0000000u) | (target << 2u);
      state_.branch_taken = true;
      state_.branch_target = state_.next_pc;
      break;
    case 0x04:
      if (state_.gpr[rs] == state_.gpr[rt]) {
        state_.next_pc = pc + 4u + (sign_extend16(imm) << 2u);
        state_.branch_taken = true;
        state_.branch_target = state_.next_pc;
      }
      break;
    case 0x05:
      if (state_.gpr[rs] != state_.gpr[rt]) {
        state_.next_pc = pc + 4u + (sign_extend16(imm) << 2u);
        state_.branch_taken = true;
        state_.branch_target = state_.next_pc;
      }
      break;
    case 0x09:
      state_.gpr[rt] = state_.gpr[rs] + sign_extend16(imm);
      break;
    case 0x0d:
      state_.gpr[rt] = state_.gpr[rs] | imm;
      break;
    case 0x0f:
      state_.gpr[rt] = imm << 16u;
      break;
    case 0x23:
      state_.gpr[rt] = read_physical(state_.gpr[rs] + sign_extend16(imm));
      break;
    case 0x2b:
      write_physical(state_.gpr[rs] + sign_extend16(imm), state_.gpr[rt]);
      break;
    default:
      O2E_WARN(LogCategory::CPU, "Unsupported opcode 0x{:08x} at 0x{:08x}",
               instr, pc);
      break;
    }

    if (state_.exception_pending) {
      handle_exception();
    }
  }

  void handle_exception() {
    if (state_.cp0.status & kMipsStatusEXL) {
      return;
    }

    const uint32_t vector =
        (state_.cp0.status & kMipsStatusBE) ? 0x80000180u : 0x80000000u;
    state_.cp0.status |= kMipsStatusEXL;
    state_.cp0.cause =
        (state_.cp0.cause & ~kMipsCauseExcCodeMask) |
        ((state_.exception_code & 0x1fu) << kMipsCauseExcCodeShift);
    state_.cp0.epc = state_.pc;
    state_.next_pc = vector;
    state_.pc = vector;
    state_.exception_pending = false;
    O2E_WARN(LogCategory::CPU,
             "Exception code={} at pc=0x{:08x}, vector=0x{:08x}",
             state_.exception_code, state_.pc, vector);
  }

  static const char *model_name(CpuModel model) {
    switch (model) {
    case CpuModel::R5000:
      return "R5000";
    case CpuModel::R10000:
      return "R10000";
    case CpuModel::R12000:
      return "R12000";
    }
    return "unknown";
  }

private:
  SystemBus *bus_ = nullptr;
  CpuState state_;
  bool running_ = false;
  CpuModel model_ = CpuModel::R5000;
  std::unordered_set<uint32_t> breakpoints_;
};

// CPU factory
std::unique_ptr<CpuInterface> create_cpu(const std::string &type) {
  if (type == "interpreter" || type == "r5000") {
    return std::make_unique<InterpreterCpu>(CpuModel::R5000);
  }
  if (type == "r10000") {
    return std::make_unique<InterpreterCpu>(CpuModel::R10000);
  }
  if (type == "r12000" || type == "r120000") {
    return std::make_unique<InterpreterCpu>(CpuModel::R12000);
  }
  O2E_ERROR(LogCategory::CPU, "Unknown CPU type '{}'", type);
  return nullptr;
}

} // namespace o2emu