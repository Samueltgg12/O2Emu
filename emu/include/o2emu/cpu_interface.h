#pragma once

#include "bus.h"
#include <array>
#include <cstdint>
#include <functional>

namespace o2emu {

enum class CpuModel : uint8_t { R5000 = 0, R10000 = 1, R12000 = 2 };

struct TlbEntry {
  bool valid = false;
  uint32_t asid = 0;
  uint32_t vpn = 0;
  uint32_t page_mask = 0;
  uint32_t pfn = 0;
};

// MIPS CPU state
struct CpuState {
  // CPU identity
  CpuModel model = CpuModel::R5000;

  struct CacheState {
    bool icache_enabled = true;
    bool dcache_enabled = true;
    uint32_t icache_linesize = 32;
    uint32_t dcache_linesize = 32;
    uint32_t icache_size = 32 * 1024;
    uint32_t dcache_size = 32 * 1024;
  } cache;

  std::array<TlbEntry, 64> tlb{};

  // General purpose registers
  std::array<uint32_t, 32> gpr{};
  uint32_t hi = 0;
  uint32_t lo = 0;

  // Program counter
  uint32_t pc = 0;
  uint32_t next_pc = 0;

  // CP0 registers (system control coprocessor)
  struct Cp0 {
    uint32_t index = 0;
    uint32_t random = 0;
    uint32_t entrylo0 = 0;
    uint32_t entrylo1 = 0;
    uint32_t context = 0;
    uint32_t pagemask = 0;
    uint32_t wired = 0;
    uint32_t badvaddr = 0;
    uint32_t count = 0;
    uint32_t entryhi = 0;
    uint32_t compare = 0;
    uint32_t status = 0;
    uint32_t cause = 0;
    uint32_t epc = 0;
    uint32_t prid = 0;
    uint32_t config = 0;
    uint32_t lladdr = 0;
    uint32_t watchlo = 0;
    uint32_t watchhi = 0;
    uint32_t xcontext = 0;
    uint32_t perf = 0;
    uint32_t ecc = 0;
    uint32_t cacheerr = 0;
    uint32_t taglo = 0;
    uint32_t taghi = 0;
    uint32_t errorepc = 0;
  } cp0;

  // FPU registers (CP1)
  std::array<uint32_t, 32> fpr{};
  uint32_t fcr0 = 0;
  uint32_t fcr31 = 0;

  // Execution state
  bool in_delay_slot = false;
  uint32_t delay_slot_pc = 0;
  bool branch_taken = false;
  uint32_t branch_target = 0;

  // Exception state
  bool exception_pending = false;
  uint32_t exception_code = 0;
  uint32_t exception_badvaddr = 0;

  // Cycle counting
  uint64_t cycles = 0;
  uint64_t instructions = 0;
};

// CPU interface - abstract base for different CPU implementations
class CpuInterface {
public:
  virtual ~CpuInterface() = default;

  // Initialize CPU with bus
  virtual void init(SystemBus *bus) = 0;

  // Reset CPU to initial state
  virtual void reset(uint32_t reset_vector = 0xbfc00000) = 0;

  // Execute one instruction
  virtual void step() = 0;

  // Execute N instructions
  virtual void run(uint64_t max_instructions = UINT64_MAX) = 0;

  // Stop execution
  virtual void stop() = 0;

  // Get/set CPU state
  virtual const CpuState &state() const = 0;
  virtual CpuState &mutable_state() = 0;

  // Interrupt handling
  virtual void raise_interrupt(uint32_t irq) = 0;
  virtual void lower_interrupt(uint32_t irq) = 0;

  // Memory access (for DMA, etc.)
  virtual uint32_t read_physical(uint32_t addr) = 0;
  virtual void write_physical(uint32_t addr, uint32_t val) = 0;

  // Breakpoint support
  virtual void add_breakpoint(uint32_t addr) = 0;
  virtual void remove_breakpoint(uint32_t addr) = 0;
  virtual bool has_breakpoint(uint32_t addr) const = 0;

  // Watchpoint support
  virtual void add_watchpoint(uint32_t addr, uint32_t size, bool read,
                              bool write) = 0;
  virtual void remove_watchpoint(uint32_t addr) = 0;

  // MMU / TLB
  virtual void add_tlb_entry(const TlbEntry &entry) = 0;
  virtual void remove_tlb_entry(uint32_t vpn) = 0;
  virtual uint32_t translate_address(uint32_t vaddr) const = 0;

  // Cache model
  virtual void set_cache_mode(bool icache_enabled, bool dcache_enabled) = 0;

  // Disassembly
  virtual std::string disassemble(uint32_t pc, uint32_t instr) const = 0;
};

// CPU factory
std::unique_ptr<CpuInterface>
create_cpu(const std::string &type = "interpreter");

} // namespace o2emu