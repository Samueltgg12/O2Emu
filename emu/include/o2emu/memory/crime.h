#pragma once

/**
 * @file crime.h
 * @brief CRIME (CPU/RAM/IO Memory Engine) memory controller
 *
 * Based on Linux arch/mips/include/asm/ip32/crime.h and IRIX sources
 */

#include <array>
#include <o2emu/o2emu.h>

namespace o2emu::memory {

class Memory;

class CRIME {
public:
  explicit CRIME(Memory &memory);
  ~CRIME() = default;

  // CRIME register indices (register space is 64KB = 0x10000 bytes = 0x4000 u32
  // registers)
  // Byte offsets from CRM_BASEADDR (Linux crime.h / IRIX crime.h).
  enum Register : uint32_t {
    REG_ID = 0x00,
    REG_CONTROL = 0x08,
    REG_INTSTAT = 0x10,
    REG_INTMASK = 0x18,
    REG_SOFTINT = 0x20,
    REG_HARDINT = 0x28,
    REG_DOG = 0x30,
    REG_TIME = 0x38,
    REG_CPU_ERROR_ADDR = 0x40,
    REG_CPU_ERROR_STAT = 0x48,
    REG_CPU_ERROR_ENA = 0x50,
    REG_VICE_ERROR_ADDR = 0x58,
    REG_MEM_CONTROL = 0x200,
    REG_MEM_BANK_CTRL0 = 0x208,
    REG_MEM_REFRESH_CNTR = 0x248,
    REG_MEM_ERROR_STAT = 0x250,
    REG_MEM_ERROR_ADDR = 0x258,
    REG_MEM_ERROR_ECC_SYN = 0x260,
    REG_MEM_ERROR_ECC_CHK = 0x268,
    REG_MEM_ERROR_ECC_REPL = 0x270,

    // Compat aliases used by older call sites
    REG_CONFIG = REG_CONTROL,
    REG_STATUS = REG_INTSTAT,
    REG_INT_STATUS = REG_INTSTAT,
    REG_INT_MASK = REG_INTMASK,
    REG_INT_CLEAR = REG_SOFTINT,
    REG_REFRESH = REG_MEM_REFRESH_CNTR,
    REG_ECC_CTRL = REG_MEM_CONTROL,
    REG_ECC_STATUS = REG_MEM_ERROR_STAT,
    REG_ECC_ADDR = REG_MEM_ERROR_ADDR,
    REG_ECC_SYNDROME = REG_MEM_ERROR_ECC_SYN,
    REG_MEM_CONFIG0 = REG_MEM_BANK_CTRL0,
    REG_MEM_CONFIG1 = REG_MEM_BANK_CTRL0 + 8,
    REG_MEM_CONFIG2 = REG_MEM_BANK_CTRL0 + 16,
    REG_MEM_CONFIG3 = REG_MEM_BANK_CTRL0 + 24,
    REG_DMA_BASE = 0x400,
    REG_TIMER_BASE = 0x500,
    REG_REVISION = 0x00,
  };

  // REG_CONTROL bits
  enum ControlBit : uint32_t {
    CTRL_MEM_ENABLE = 0,
    CTRL_ECC_ENABLE = 1,
    CTRL_SCRUB_ENABLE = 2,
    CTRL_REFRESH_ENABLE = 3,
  };

  // REG_INT_STATUS/REG_INT_MASK bits
  enum InterruptBit : uint32_t {
    INTR_TIMER_0 = 8,
    INTR_TIMER_1 = 9,
    INTR_DMA_0 = 16,
    INTR_DMA_1 = 17,
    INTR_DMA_2 = 18,
    INTR_DMA_3 = 19,
    INTR_DMA_4 = 20,
    INTR_DMA_5 = 21,
    INTR_DMA_6 = 22,
    INTR_DMA_7 = 23,
  };

  // Read/write registers (by byte offset)
  u32 read(u32 offset) const;
  void write(u32 offset, u32 value);

  // Memory configuration
  void set_bank_config(int bank, u32 base, u32 size_mb, bool enabled);
  u32 get_bank_config(int bank) const;
  int num_banks() const { return num_banks_; }
  u32 total_memory_mb() const { return total_mem_mb_; }

  // ECC
  void enable_ecc(bool enable) { ecc_enabled_ = enable; }
  bool ecc_enabled() const { return ecc_enabled_; }
  void inject_ecc_error(u32 addr, u32 syndrome);

  // Refresh
  void set_refresh_rate(u32 cycles_per_refresh);

  // Interrupts
  void set_interrupt(InterruptBit bit, bool asserted);
  u32 interrupt_status() const;
  u32 interrupt_mask() const { return intr_mask_; }
  void set_interrupt_mask(u32 mask) { intr_mask_ = mask; }
  u32 pending_interrupts() const { return intr_status_ & intr_mask_; }

  // Timer
  void tick_timers();
  void tick(u64 cycles);

  // DMA
  void start_dma(u32 channel);

  // Interrupt handling
  void clear_interrupt(u32 bit);

  // Reset
  void reset();

private:
  Memory &memory_;
  std::array<u32, 0x10000 / 4> regs_ = {}; // 64KB register space

  // Bank configuration
  struct BankConfig {
    u32 base = 0;
    u32 size_mb = 0;
    bool enabled = false;
  };
  std::array<BankConfig, 8> banks_ = {};
  int num_banks_ = 0;
  u32 total_mem_mb_ = 0;
  bool ecc_enabled_ = false;

  // Interrupts
  u32 intr_status_ = 0;
  u32 intr_mask_ = 0;

  // Timers
  u32 timer0_count_ = 0;
  u32 timer0_compare_ = 0;
  u32 timer1_count_ = 0;
  u32 timer1_compare_ = 0;

  // Refresh
  u64 refresh_counter_ = 0;

  // Internal handlers
  void handle_control_write(u32 value);
  void handle_dma_write(u32 reg_offset, u32 value);
  void handle_timer_write(u32 reg_offset, u32 value);
};

} // namespace o2emu::memory