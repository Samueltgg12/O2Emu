/**
 * @file cp0.cpp
 * @brief CP0 (System Control Coprocessor) implementation
 */

#include <o2emu/cpu/cp0.h>
#include <o2emu/logging/logger.h>

namespace o2emu::cpu {

void CP0::reset() {
  regs_.fill(0);

  // PRId register - R5000
  regs_[Register::PRID] = 0x00002700; // R5000, revision 0

  // Config register
  regs_[Register::CONFIG] = 0x0006E463; // Kseg0 cacheable, 32-bit, etc.

  // Status register - BEV=1 (bootstrap exception vectors), KU=0 (kernel), IE=0
  regs_[Register::STATUS] = 0x00000400;

  // Cause register
  regs_[Register::CAUSE] = 0;

  // Count/Compare
  regs_[Register::COUNT] = 0;
  regs_[Register::COMPARE] = 0xFFFFFFFF;

  // EPC
  regs_[Register::EPC] = 0;

  // BadVAddr
  regs_[Register::BADVADDR] = 0;

  // Context
  regs_[Register::CONTEXT] = 0;

  // EntryHi/EntryLo
  regs_[Register::ENTRYHI] = 0;
  regs_[Register::ENTRYLO0] = 0;
  regs_[Register::ENTRYLO1] = 0;

  // PageMask
  regs_[Register::PAGEMASK] = 0;

  // Wired
  regs_[Register::WIRED] = 0;

  // Index/Random
  regs_[Register::INDEX] = 0;
  regs_[Register::RANDOM] = 63; // 64 TLB entries - 1

  // WatchLo/WatchHi
  regs_[Register::WATCHLO] = 0;
  regs_[Register::WATCHHI] = 0;

  // XContext
  regs_[Register::XCONTEXT] = 0;

  // Cache Error
  regs_[Register::CACHEERR] = 0;

  // TagLo/TagHi
  regs_[Register::TAGLO] = 0;
  regs_[Register::TAGHI] = 0;

  // ErrorEPC
  regs_[Register::ERROREPC] = 0;

  // DESAVE
  regs_[Register::DESAVE] = 0;
}

u32 CP0::read(Register reg, int sel) const {
  (void)sel; // Select not used for most registers
  return regs_[static_cast<uint32_t>(reg)];
}

void CP0::write(Register reg, u32 value, int sel) {
  (void)sel; // Select not used for most registers
  if (reg >= 32)
    return;

  switch (reg) {
  case Register::STATUS:
    // Some bits are read-only or have special behavior
    regs_[reg] = (value & 0xFFFCFFFF) | (regs_[reg] & 0x00030000);
    break;
  case Register::CAUSE:
    // Only IP1-IP0 (software interrupts) are writable
    regs_[reg] = (regs_[reg] & ~0x00000300) | (value & 0x00000300);
    break;
  case Register::COUNT:
    regs_[reg] = value;
    break;
  case Register::COMPARE:
    regs_[reg] = value;
    break;
  case Register::EPC:
    regs_[reg] = value;
    break;
  case Register::PRID:
  case Register::CONFIG:
    // Read-only
    break;
  default:
    regs_[reg] = value;
    break;
  }
}

void CP0::tick(u64 cycles) {
  regs_[Register::COUNT] += static_cast<u32>(cycles);

  // Check for timer interrupt
  if (regs_[Register::COUNT] >= regs_[Register::COMPARE]) {
    regs_[Register::CAUSE] |= 0x8000; // IP7 (timer interrupt)
  }
}

void CP0::exception(ExceptionCode exc, CPUState &cpu_state) {
  u32 exc_code = static_cast<u32>(exc);

  // Set EXL bit in Status
  regs_[Register::STATUS] |= 0x00000002;

  // Set exception code in Cause
  regs_[Register::CAUSE] =
      (regs_[Register::CAUSE] & ~0x0000007C) | (exc_code << 2);

  // Save PC in EPC
  if (regs_[Register::STATUS] & 0x00000002) { // EXL already set
    regs_[Register::ERROREPC] = cpu_state.pc;
  } else {
    regs_[Register::EPC] = cpu_state.pc;
  }

  // Set BadVAddr for address-related exceptions
  if (exc == Exception::ADEL || exc == Exception::ADES ||
      exc == Exception::TLBL || exc == Exception::TLBS ||
      exc == Exception::MOD) {
    regs_[Register::BADVADDR] = cpu_state.pc; // Should be the faulting address
  }

  // Set BD bit if in branch delay slot
  // (simplified - we don't track this)

  // Vector to exception handler
  if (regs_[Register::STATUS] & 0x00400000) { // BEV bit
    // Bootstrap exception vector
    cpu_state.pc = 0xBFC00200 + (exc_code * 0x80);
  } else {
    // Normal exception vector
    cpu_state.pc = 0x80000000 + (exc_code * 0x80);
  }

  O2EMU_LOG_DEBUG_F("Exception: %d, EPC=0x%08x", exc_code,
                    regs_[Register::EPC]);
}

void CP0::rfe() {
  // Restore From Exception - clear EXL bit
  regs_[Register::STATUS] &= ~0x00000002;
}

void CP0::eret(CPU::State &cpu_state) {
  // Return from Exception
  if (regs_[Register::STATUS] & 0x00000004) { // ERL bit
    cpu_state.pc = regs_[Register::ERROREPC];
    regs_[Register::STATUS] &= ~0x00000004; // Clear ERL
  } else {
    cpu_state.pc = regs_[Register::EPC];
    regs_[Register::STATUS] &= ~0x00000002; // Clear EXL
  }
}

u32 CP0::pending_interrupts() const {
  u32 cause_ip = (regs_[Register::CAUSE] >> 8) & 0xFF;   // IP7-IP0
  u32 status_im = (regs_[Register::STATUS] >> 8) & 0xFF; // IM7-IM0
  u32 status_ie = regs_[Register::STATUS] & 0x0001;      // IE bit

  if (!status_ie)
    return 0;

  return cause_ip & status_im;
}

void CP0::dump() const {
  O2EMU_LOG_INFO_F("=== CP0 Registers ===");
  O2EMU_LOG_INFO_F("Index:     0x%08x", regs_[Register::INDEX]);
  O2EMU_LOG_INFO_F("Random:    0x%08x", regs_[Register::RANDOM]);
  O2EMU_LOG_INFO_F("EntryLo0:  0x%08x", regs_[Register::ENTRYLO0]);
  O2EMU_LOG_INFO_F("EntryLo1:  0x%08x", regs_[Register::ENTRYLO1]);
  O2EMU_LOG_INFO_F("Context:   0x%08x", regs_[Register::CONTEXT]);
  O2EMU_LOG_INFO_F("PageMask:  0x%08x", regs_[Register::PAGEMASK]);
  O2EMU_LOG_INFO_F("Wired:     0x%08x", regs_[Register::WIRED]);
  O2EMU_LOG_INFO_F("BadVAddr:  0x%08x", regs_[Register::BADVADDR]);
  O2EMU_LOG_INFO_F("Count:     0x%08x", regs_[Register::COUNT]);
  O2EMU_LOG_INFO_F("EntryHi:   0x%08x", regs_[Register::ENTRYHI]);
  O2EMU_LOG_INFO_F("Compare:   0x%08x", regs_[Register::COMPARE]);
  O2EMU_LOG_INFO_F("Status:    0x%08x", regs_[Register::STATUS]);
  O2EMU_LOG_INFO_F("Cause:     0x%08x", regs_[Register::CAUSE]);
  O2EMU_LOG_INFO_F("EPC:       0x%08x", regs_[Register::EPC]);
  O2EMU_LOG_INFO_F("PRId:      0x%08x", regs_[Register::PRID]);
  O2EMU_LOG_INFO_F("Config:    0x%08x", regs_[Register::CONFIG]);
  O2EMU_LOG_INFO_F("LLAddr:    0x%08x", regs_[Register::LLADDR]);
  O2EMU_LOG_INFO_F("WatchLo:   0x%08x", regs_[Register::WATCHLO]);
  O2EMU_LOG_INFO_F("WatchHi:   0x%08x", regs_[Register::WATCHHI]);
  O2EMU_LOG_INFO_F("XContext:  0x%08x", regs_[Register::XCONTEXT]);
  O2EMU_LOG_INFO_F("Parity:    0x%08x", regs_[Register::PARITY]);
  O2EMU_LOG_INFO_F("CacheErr:  0x%08x", regs_[Register::CACHEERR]);
  O2EMU_LOG_INFO_F("TagLo:     0x%08x", regs_[Register::TAGLO]);
  O2EMU_LOG_INFO_F("TagHi:     0x%08x", regs_[Register::TAGHI]);
  O2EMU_LOG_INFO_F("ErrorEPC:  0x%08x", regs_[Register::ERROREPC]);
  O2EMU_LOG_INFO_F("DESAVE:    0x%08x", regs_[Register::DESAVE]);
}

} // namespace o2emu::cpu