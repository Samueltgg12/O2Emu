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
  enum Register : uint32_t {
    // Core registers
    REG_ID = 0x0000,
    REG_CONFIG = 0x0002,
    REG_STATUS = 0x0004,
    REG_CONTROL = 0x0006,

    // Memory configuration (4 banks)
    REG_MEM_CONFIG0 = 0x0040,
    REG_MEM_CONFIG1 = 0x0041,
    REG_MEM_CONFIG2 = 0x0042,
    REG_MEM_CONFIG3 = 0x0043,

    // Refresh control
    REG_REFRESH = 0x0048,

    // ECC control
    REG_ECC_CTRL = 0x004C,
    REG_ECC_STATUS = 0x004E,
    REG_ECC_ADDR = 0x0050,
    REG_ECC_SYNDROME = 0x0052,

    // Interrupt control
    REG_INT_STATUS = 0x0080,
    REG_INT_MASK = 0x0082,
    REG_INT_CLEAR = 0x0084,

    // DMA registers (8 channels, 6 registers each)
    REG_DMA_BASE = 0x0100,
    // Per channel: SRC, DST, COUNT, CTRL, NEXT, STATUS

    // Timer registers (2 timers, 3 registers each)
    REG_TIMER_BASE = 0x0200,
    // Per timer: COUNT, COMPARE, CTRL

    // Revision
    REG_REVISION = 0x3FFC,
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