#pragma once

/**
 * @file cp0.h
 * @brief MIPS CP0 (System Control Coprocessor) registers
 */

#include <array>
#include <o2emu/o2emu.h>

namespace o2emu::cpu {

struct CPUState; // Forward declaration

class CP0 {
public:
  CP0();
  ~CP0() = default;

  // CP0 Register indices
  enum Register : uint32_t {
    INDEX = 0,     // TLB index
    RANDOM = 1,    // TLB random
    ENTRYLO0 = 2,  // TLB entry low 0
    ENTRYLO1 = 3,  // TLB entry low 1
    CONTEXT = 4,   // TLB context
    PAGEMASK = 5,  // TLB page mask
    WIRED = 6,     // TLB wired
    BADVADDR = 8,  // Bad virtual address
    COUNT = 9,     // Timer count
    ENTRYHI = 10,  // TLB entry high
    COMPARE = 11,  // Timer compare
    STATUS = 12,   // Status register
    CAUSE = 13,    // Cause register
    EPC = 14,      // Exception PC
    PRID = 15,     // Processor revision ID
    CONFIG = 16,   // Configuration
    LLADDR = 17,   // Load linked address
    WATCHLO = 18,  // Watchpoint low
    WATCHHI = 19,  // Watchpoint high
    XCONTEXT = 20, // Extended context
    ECC = 26,      // ECC register
    CACHEERR = 27, // Cache error
    TAGLO = 28,    // Cache tag low
    TAGHI = 29,    // Cache tag high
    ERROREPC = 30, // Error exception PC
  };

  // Status register bits
  struct StatusBits {
    u32 IE : 1;  // Interrupt enable (global)
    u32 EXL : 1; // Exception level
    u32 ERL : 1; // Error level
    u32 KSU : 2; // Kernel/Supervisor/User mode
    u32 UX : 1;  // User mode 64-bit addressing
    u32 SX : 1;  // Supervisor mode 64-bit addressing
    u32 KX : 1;  // Kernel mode 64-bit addressing
    u32 IM : 8;  // Interrupt mask (8 lines)
    u32 DS : 1;  // Debug single step
    u32 : 1;     // Reserved
    u32 RE : 1;  // Reverse endian (user)
    u32 : 1;     // Reserved
    u32 BEV : 1; // Bootstrap exception vectors
    u32 TS : 1;  // TLB shutdown
    u32 SR : 1;  // Soft reset
    u32 NMI : 1; // NMI occurred
    u32 : 9;     // Reserved
  };

  // Cause register bits
  struct CauseBits {
    u32 : 8;         // Reserved
    u32 IP : 8;      // Interrupt pending
    u32 : 1;         // Reserved
    u32 CE : 2;      // Coprocessor error
    u32 : 1;         // Reserved
    u32 EXCCODE : 5; // Exception code
    u32 : 1;         // Reserved
    u32 BD : 1;      // Branch delay slot
    u32 : 6;         // Reserved
  };

  // Config register bits (R5000)
  struct ConfigBits {
    u32 K0 : 3; // Kseg0 coherency algorithm
    u32 : 1;    // Reserved
    u32 CU : 1; // Coprocessor usable
    u32 : 1;    // Reserved
    u32 BE : 1; // Big endian
    u32 : 1;    // Reserved
    u32 AT : 2; // Architecture type
    u32 AR : 2; // Architecture revision
    u32 MT : 2; // MMU type
    u32 : 1;    // Reserved
    u32 VI : 1; // Virtual instruction cache
    u32 : 1;    // Reserved
    u32 EP : 1; // EJTAG present
    u32 : 16;   // Reserved
  };

  // Read/write CP0 register
  u32 read(Register reg, int sel = 0);
  void write(Register reg, u32 value, int sel = 0);

  // Direct access to key registers
  u32 status() const { return regs_[STATUS]; }
  void set_status(u32 v) { regs_[STATUS] = v; }

  u32 cause() const { return regs_[CAUSE]; }
  void set_cause(u32 v) { regs_[CAUSE] = v; }

  u32 epc() const { return regs_[EPC]; }
  void set_epc(u32 v) { regs_[EPC] = v; }

  u32 badvaddr() const { return regs_[BADVADDR]; }
  void set_badvaddr(u32 v) { regs_[BADVADDR] = v; }

  u32 count() const { return regs_[COUNT]; }
  void set_count(u32 v) { regs_[COUNT] = v; }

  u32 compare() const { return regs_[COMPARE]; }
  void set_compare(u32 v) { regs_[COMPARE] = v; }

  u32 config() const { return regs_[CONFIG]; }

  u32 prid() const { return regs_[PRID]; }

  // Interrupt handling
  void set_interrupt_pending(int line, bool pending);
  u32 interrupt_pending() const { return (regs_[CAUSE] >> 8) & 0xFF; }
  u32 interrupt_mask() const { return (regs_[STATUS] >> 8) & 0xFF; }
  void set_interrupt_mask(u32 mask) {
    regs_[STATUS] = (regs_[STATUS] & ~0xFF00) | ((mask & 0xFF) << 8);
  }

  // Timer
  void tick_timer();
  bool timer_interrupt_pending() const;

  // Mode
  bool is_kernel_mode() const { return (regs_[STATUS] & 0x3F) == 0; }
  bool is_user_mode() const { return (regs_[STATUS] & 0x10) != 0; }
  bool interrupts_enabled() const { return (regs_[STATUS] & 0x1) != 0; }

  // Reset
  void reset();

  // Exception handling
  void exception(ExceptionCode exc, CPUState &cpu_state);
  void rfe();
  void eret(CPUState &cpu_state);

  // Timer tick
  void tick(u64 cycles);

  // Interrupt handling
  u32 pending_interrupts() const;

  // Debugging
  void dump() const;

private:
  std::array<u32, 32> regs_ = {};

  // TLB (simplified for now)
  struct TLBEntry {
    u32 entry_hi = 0;
    u32 entry_lo0 = 0;
    u32 entry_lo1 = 0;
    u32 page_mask = 0;
    bool valid = false;
  };
  std::array<TLBEntry, 48> tlb_; // R5000 has 48 TLB entries
  u32 tlb_index_ = 0;
  u32 tlb_random_ = 47;
  u32 tlb_wired_ = 0;
};

} // namespace o2emu::cpu