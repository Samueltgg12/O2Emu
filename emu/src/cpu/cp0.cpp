/**
 * @file cp0.cpp
 * @brief CP0 (System Control Coprocessor) implementation
 */

#include <cstring>
#include <o2emu/cpu/cp0.h>
#include <o2emu/logging/logger.h>

namespace o2emu::cpu {

CP0::CP0() { reset(); }

CP0::~CP0() = default;

void CP0::reset() {
  std::memset(regs_, 0, sizeof(regs_));

  // PRId register - R5000
  regs_[REG_PRID] = 0x00002700; // R5000, revision 0

  // Config register
  regs_[REG_CONFIG] = 0x0006E463; // Kseg0 cacheable, 32-bit, etc.

  // Status register - BEV=1 (bootstrap exception vectors), KU=0 (kernel), IE=0
  regs_[REG_STATUS] = 0x00000400;

  // Cause register
  regs_[REG_CAUSE] = 0;

  // Count/Compare
  regs_[REG_COUNT] = 0;
  regs_[REG_COMPARE] = 0xFFFFFFFF;

  // EPC
  regs_[REG_EPC] = 0;

  // BadVAddr
  regs_[REG_BADVADDR] = 0;

  // Context
  regs_[REG_CONTEXT] = 0;

  // EntryHi/EntryLo
  regs_[REG_ENTRYHI] = 0;
  regs_[REG_ENTRYLO0] = 0;
  regs_[REG_ENTRYLO1] = 0;

  // PageMask
  regs_[REG_PAGEMASK] = 0;

  // Wired
  regs_[REG_WIRED] = 0;

  // Index/Random
  regs_[REG_INDEX] = 0;
  regs_[REG_RANDOM] = 63; // 64 TLB entries - 1

  // WatchLo/WatchHi
  regs_[REG_WATCHLO] = 0;
  regs_[REG_WATCHHI] = 0;

  // XContext
  regs_[REG_XCONTEXT] = 0;

  // Parity Error
  regs_[REG_PARITY] = 0;

  // Cache Error
  regs_[REG_CACHEERR] = 0;

  // TagLo/TagHi
  regs_[REG_TAGLO] = 0;
  regs_[REG_TAGHI] = 0;

  // ErrorEPC
  regs_[REG_ERROREPC] = 0;

  // DESAVE
  regs_[REG_DESAVE] = 0;
}

u32 CP0::read(u32 reg) const {
  if (reg < 32) {
    return regs_[reg];
  }
  return 0;
}

void CP0::write(u32 reg, u32 value) {
  if (reg >= 32)
    return;

  switch (reg) {
  case REG_STATUS:
    // Some bits are read-only or have special behavior
    regs_[reg] = (value & 0xFFFCFFFF) | (regs_[reg] & 0x00030000);
    break;
  case REG_CAUSE:
    // Only IP1-IP0 (software interrupts) are writable
    regs_[reg] = (regs_[reg] & ~0x00000300) | (value & 0x00000300);
    break;
  case REG_COUNT:
    regs_[reg] = value;
    break;
  case REG_COMPARE:
    regs_[reg] = value;
    break;
  case REG_EPC:
    regs_[reg] = value;
    break;
  case REG_PRID:
  case REG_CONFIG:
    // Read-only
    break;
  default:
    regs_[reg] = value;
    break;
  }
}

void CP0::tick(u64 cycles) {
  regs_[REG_COUNT] += static_cast<u32>(cycles);

  // Check for timer interrupt
  if (regs_[REG_COUNT] >= regs_[REG_COMPARE]) {
    regs_[REG_CAUSE] |= 0x8000; // IP7 (timer interrupt)
  }
}

void CP0::exception(Exception exc, CPU::State &cpu_state) {
  u32 exc_code = static_cast<u32>(exc);

  // Set EXL bit in Status
  regs_[REG_STATUS] |= 0x00000002;

  // Set exception code in Cause
  regs_[REG_CAUSE] = (regs_[REG_CAUSE] & ~0x0000007C) | (exc_code << 2);

  // Save PC in EPC
  if (regs_[REG_STATUS] & 0x00000002) { // EXL already set
    regs_[REG_ERROREPC] = cpu_state.pc;
  } else {
    regs_[REG_EPC] = cpu_state.pc;
  }

  // Set BadVAddr for address-related exceptions
  if (exc == Exception::ADEL || exc == Exception::ADES ||
      exc == Exception::TLBL || exc == Exception::TLBS ||
      exc == Exception::MOD) {
    regs_[REG_BADVADDR] = cpu_state.pc; // Should be the faulting address
  }

  // Set BD bit if in branch delay slot
  // (simplified - we don't track this)

  // Vector to exception handler
  if (regs_[REG_STATUS] & 0x00400000) { // BEV bit
    // Bootstrap exception vector
    cpu_state.pc = 0xBFC00200 + (exc_code * 0x80);
  } else {
    // Normal exception vector
    cpu_state.pc = 0x80000000 + (exc_code * 0x80);
  }

  O2EMU_LOG_DEBUG("Exception: " << exc_code << ", EPC=0x" << std::hex
                                << regs_[REG_EPC] << std::dec);
}

void CP0::rfe() {
  // Restore From Exception - clear EXL bit
  regs_[REG_STATUS] &= ~0x00000002;
}

void CP0::eret(CPU::State &cpu_state) {
  // Return from Exception
  if (regs_[REG_STATUS] & 0x00000004) { // ERL bit
    cpu_state.pc = regs_[REG_ERROREPC];
    regs_[REG_STATUS] &= ~0x00000004; // Clear ERL
  } else {
    cpu_state.pc = regs_[REG_EPC];
    regs_[REG_STATUS] &= ~0x00000002; // Clear EXL
  }
}

u32 CP0::pending_interrupts() const {
  u32 cause_ip = (regs_[REG_CAUSE] >> 8) & 0xFF;   // IP7-IP0
  u32 status_im = (regs_[REG_STATUS] >> 8) & 0xFF; // IM7-IM0
  u32 status_ie = regs_[REG_STATUS] & 0x0001;      // IE bit

  if (!status_ie)
    return 0;

  return cause_ip & status_im;
}

void CP0::dump() const {
  O2EMU_LOG_INFO("=== CP0 Registers ===");
  O2EMU_LOG_INFO("Index:     0x" << std::hex << regs_[REG_INDEX] << std::dec);
  O2EMU_LOG_INFO("Random:    0x" << std::hex << regs_[REG_RANDOM] << std::dec);
  O2EMU_LOG_INFO("EntryLo0:  0x" << std::hex << regs_[REG_ENTRYLO0]
                                 << std::dec);
  O2EMU_LOG_INFO("EntryLo1:  0x" << std::hex << regs_[REG_ENTRYLO1]
                                 << std::dec);
  O2EMU_LOG_INFO("Context:   0x" << std::hex << regs_[REG_CONTEXT] << std::dec);
  O2EMU_LOG_INFO("PageMask:  0x" << std::hex << regs_[REG_PAGEMASK]
                                 << std::dec);
  O2EMU_LOG_INFO("Wired:     0x" << std::hex << regs_[REG_WIRED] << std::dec);
  O2EMU_LOG_INFO("BadVAddr:  0x" << std::hex << regs_[REG_BADVADDR]
                                 << std::dec);
  O2EMU_LOG_INFO("Count:     0x" << std::hex << regs_[REG_COUNT] << std::dec);
  O2EMU_LOG_INFO("EntryHi:   0x" << std::hex << regs_[REG_ENTRYHI] << std::dec);
  O2EMU_LOG_INFO("Compare:   0x" << std::hex << regs_[REG_COMPARE] << std::dec);
  O2EMU_LOG_INFO("Status:    0x" << std::hex << regs_[REG_STATUS] << std::dec);
  O2EMU_LOG_INFO("Cause:     0x" << std::hex << regs_[REG_CAUSE] << std::dec);
  O2EMU_LOG_INFO("EPC:       0x" << std::hex << regs_[REG_EPC] << std::dec);
  O2EMU_LOG_INFO("PRId:      0x" << std::hex << regs_[REG_PRID] << std::dec);
  O2EMU_LOG_INFO("Config:    0x" << std::hex << regs_[REG_CONFIG] << std::dec);
  O2EMU_LOG_INFO("LLAddr:    0x" << std::hex << regs_[REG_LLADDR] << std::dec);
  O2EMU_LOG_INFO("WatchLo:   0x" << std::hex << regs_[REG_WATCHLO] << std::dec);
  O2EMU_LOG_INFO("WatchHi:   0x" << std::hex << regs_[REG_WATCHHI] << std::dec);
  O2EMU_LOG_INFO("XContext:  0x" << std::hex << regs_[REG_XCONTEXT]
                                 << std::dec);
  O2EMU_LOG_INFO("Parity:    0x" << std::hex << regs_[REG_PARITY] << std::dec);
  O2EMU_LOG_INFO("CacheErr:  0x" << std::hex << regs_[REG_CACHEERR]
                                 << std::dec);
  O2EMU_LOG_INFO("TagLo:     0x" << std::hex << regs_[REG_TAGLO] << std::dec);
  O2EMU_LOG_INFO("TagHi:     0x" << std::hex << regs_[REG_TAGHI] << std::dec);
  O2EMU_LOG_INFO("ErrorEPC:  0x" << std::hex << regs_[REG_ERROREPC]
                                 << std::dec);
  O2EMU_LOG_INFO("DESAVE:    0x" << std::hex << regs_[REG_DESAVE] << std::dec);
}

} // namespace o2emu::cpu