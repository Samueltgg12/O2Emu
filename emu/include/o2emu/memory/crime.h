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

class CRIME {
public:
  CRIME();
  ~CRIME() = default;

  // CRIME register offsets (from PHYS_BASE_CRIME = 0x14000000)
  enum Register : uint32_t {
    // Memory control
    CRM_MEM_CONFIG = 0x000000,
    CRM_MEM_BANK_CTRL = 0x000004,
    CRM_MEM_REFRESH = 0x000008,
    CRM_MEM_ECC_CTRL = 0x00000C,
    CRM_MEM_ECC_STATUS = 0x000010,
    CRM_MEM_ECC_ADDR = 0x000014,
    CRM_MEM_ECC_SYNDROME = 0x000018,

    // Interrupt control
    CRM_INTR_STATUS = 0x000100,
    CRM_INTR_MASK = 0x000104,
    CRM_INTR_CLEAR = 0x000108,
    CRM_INTR_SET = 0x00010C,

    // PCI/IO
    CRM_PCI_CONFIG = 0x000200,
    CRM_PCI_IO_BASE = 0x000204,
    CRM_PCI_MEM_BASE = 0x000208,

    // Revision
    CRM_REVISION = 0x000FFC,
  };

  // CRM_MEM_CONFIG bits
  struct MemConfigBits {
    u32 banks : 4;      // Number of banks populated
    u32 bank_size : 3;  // Bank size encoding
    u32 ecc_enable : 1; // ECC enable
    u32 : 24;           // Reserved
  };

  // CRM_MEM_BANK_CTRL bits (per bank)
  struct BankCtrlBits {
    u32 base_addr : 12; // Bank base address (256MB granularity)
    u32 size : 3;       // Bank size
    u32 enabled : 1;    // Bank enabled
    u32 : 16;           // Reserved
  };

  // CRM_INTR_STATUS/CRM_INTR_MASK bits
  enum InterruptBit : uint32_t {
    INTR_PCI_ERROR = 0,
    INTR_PCI_SERR = 1,
    INTR_PCI_PERR = 2,
    INTR_MEMORY_ECC = 3,
    INTR_MEMORY_REFRESH = 4,
    INTR_TIMER_0 = 5,
    INTR_TIMER_1 = 6,
    INTR_UART_1 = 7,
    INTR_UART_2 = 8,
    INTR_RTC = 9,
    INTR_PS2_KEYBOARD = 10,
    INTR_PS2_MOUSE = 11,
    INTR_ETHERNET = 12,
    INTR_AUDIO = 13,
    INTR_SCSI_0 = 14,
    INTR_SCSI_1 = 15,
    INTR_GRAPHICS = 16,
    INTR_VERTICAL_RETRACE = 17,
  };

  // Read/write registers
  u32 read(Register reg);
  void write(Register reg, u32 value);

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
  u32 interrupt_status() const { return intr_status_; }
  u32 interrupt_mask() const { return intr_mask_; }
  void set_interrupt_mask(u32 mask) { intr_mask_ = mask; }
  u32 pending_interrupts() const { return intr_status_ & intr_mask_; }

  // Timer
  void tick_timers();

  // Reset
  void reset();

private:
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
};

} // namespace o2emu::memory