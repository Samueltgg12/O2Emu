#pragma once

/**
 * @file mace_isa.h
 * @brief MACE ISA External (UART, RTC, PS/2, Parallel)
 *
 * Offset from MACE base: 0x380000 (MACE_ISA_EXTERNAL_OFFSET)
 * Based on PROM decompiled definitions.h
 */

#include <array>
#include <o2emu/devices/device.h>
#include <o2emu/o2emu.h>
#include <queue>

namespace o2emu::devices {

class MACEISA : public Device {
public:
  MACEISA();
  ~MACEISA() override;

  // ISA External register offsets (from MACE base + 0x380000)
  enum Register : uint32_t {
    // UART 1 (16550-compatible) at offset 0x10000
    UART1_BASE = 0x10000,
    UART1_RBR = 0x10000, // Receive Buffer Register (read)
    UART1_THR = 0x10000, // Transmit Holding Register (write)
    UART1_IER = 0x10004, // Interrupt Enable Register
    UART1_IIR = 0x10008, // Interrupt Identification Register (read)
    UART1_FCR = 0x10008, // FIFO Control Register (write)
    UART1_LCR = 0x1000C, // Line Control Register
    UART1_MCR = 0x10010, // Modem Control Register
    UART1_LSR = 0x10014, // Line Status Register
    UART1_MSR = 0x10018, // Modem Status Register
    UART1_SCR = 0x1001C, // Scratch Register
    UART1_DLL = 0x10000, // Divisor Latch Low (LCR[7]=1)
    UART1_DLM = 0x10004, // Divisor Latch High (LCR[7]=1)

    // UART 2 at offset 0x18000
    UART2_BASE = 0x18000,
    UART2_RBR = 0x18000,
    UART2_THR = 0x18000,
    UART2_IER = 0x18004,
    UART2_IIR = 0x18008,
    UART2_FCR = 0x18008,
    UART2_LCR = 0x1800C,
    UART2_MCR = 0x18010,
    UART2_LSR = 0x18014,
    UART2_MSR = 0x18018,
    UART2_SCR = 0x1801C,
    UART2_DLL = 0x18000,
    UART2_DLM = 0x18004,

    // RTC (DS12887-compatible) at offset 0x20000
    RTC_BASE = 0x20000,

    // RTC standard registers (indices, combined with RTC_BASE + (index << 8))
    RTC_SECONDS = 0x00,
    RTC_SECONDS_ALARM = 0x01,
    RTC_MINUTES = 0x02,
    RTC_MINUTES_ALARM = 0x03,
    RTC_HOURS = 0x04,
    RTC_HOURS_ALARM = 0x05,
    RTC_DAY_OF_WEEK = 0x06,
    RTC_DAY_OF_MONTH = 0x07,
    RTC_MONTH = 0x08,
    RTC_YEAR = 0x09,
    RTC_REG_A = 0x0A,
    RTC_REG_B = 0x0B,
    RTC_REG_C = 0x0C,
    RTC_REG_D = 0x0D,

    // PS/2 Keyboard at offset 0x28000
    PS2_KBD_BASE = 0x28000,
    PS2_KBD_DATA = 0x28000,
    PS2_KBD_STATUS = 0x28004,
    PS2_KBD_CTRL = 0x28008,

    // PS/2 Mouse at offset 0x28100
    PS2_MOUSE_BASE = 0x28100,
    PS2_MOUSE_DATA = 0x28100,
    PS2_MOUSE_STATUS = 0x28104,
    PS2_MOUSE_CTRL = 0x28108,

    // Parallel port at offset 0x30000
    PARALLEL_BASE = 0x30000,
    PARALLEL_DATA = 0x30000,
    PARALLEL_STATUS = 0x30004,
    PARALLEL_CTRL = 0x30008,
  };

  // Calculate RTC register offset from index (registers at 256-byte intervals)
  static constexpr u32 rtc_reg_offset(u8 index) {
    return RTC_BASE + (static_cast<u32>(index) << 8);
  }

  // UART LCR bits
  enum UartLcrBit : uint8_t {
    UART_LCR_DLAB = 0x80,        // Divisor Latch Access Bit
    UART_LCR_SB = 0x40,          // Set Break
    UART_LCR_PARITY_MASK = 0x38, // Parity bits
    UART_LCR_STOP = 0x04,        // Stop bits
    UART_LCR_WORD_MASK = 0x03,   // Word length
  };

  // UART LSR bits
  enum UartLsrBit : uint8_t {
    UART_LSR_DR = 0x01,       // Data Ready
    UART_LSR_OE = 0x02,       // Overrun Error
    UART_LSR_PE = 0x04,       // Parity Error
    UART_LSR_FE = 0x08,       // Framing Error
    UART_LSR_BI = 0x10,       // Break Interrupt
    UART_LSR_THRE = 0x20,     // THR Empty
    UART_LSR_TEMT = 0x40,     // Transmitter Empty
    UART_LSR_FIFO_ERR = 0x80, // FIFO Error
  };

  // UART IER bits
  enum UartIerBit : uint8_t {
    UART_IER_RDI = 0x01,  // Receive Data Interrupt
    UART_IER_THRI = 0x02, // THR Empty Interrupt
    UART_IER_RLSI = 0x04, // Receiver Line Status Interrupt
    UART_IER_MSI = 0x08,  // Modem Status Interrupt
  };

  // RTC Register A bits
  enum RtcRegABit : uint8_t {
    RTC_A_UIP = 0x80,     // Update In Progress
    RTC_A_DV_MASK = 0x70, // Divider
    RTC_A_RS_MASK = 0x0F, // Rate Select
  };

  // RTC Register B bits
  enum RtcRegBBit : uint8_t {
    RTC_B_SET = 0x80,  // Set mode (stop updates)
    RTC_B_PIE = 0x40,  // Periodic Interrupt Enable
    RTC_B_AIE = 0x20,  // Alarm Interrupt Enable
    RTC_B_UIE = 0x10,  // Update Ended Interrupt Enable
    RTC_B_SQWE = 0x08, // Square Wave Enable
    RTC_B_DM = 0x04,   // Data Mode (0=BCD, 1=Binary)
    RTC_B_24H = 0x02,  // 24-hour mode
    RTC_B_DSE = 0x01,  // Daylight Savings Enable
  };

  // Device interface
  u32 read32(u32 offset) override;
  u16 read16(u32 offset) override;
  u8 read8(u32 offset) override;

  void write32(u32 offset, u32 value) override;
  void write16(u32 offset, u16 value) override;
  void write8(u32 offset, u8 value) override;

  void reset() override;
  void tick(u64 cycles) override;

  u32 interrupt_status() const override;
  void clear_interrupt(u32 mask) override;

  // Configuration write (called by MACE for ISA config space)
  void config_write(u32 reg, u32 value);

  // UART callbacks
  using UartCallback = std::function<void(u8 data)>;
  void set_uart1_output(UartCallback cb) { uart1_output_cb_ = std::move(cb); }
  void set_uart2_output(UartCallback cb) { uart2_output_cb_ = std::move(cb); }
  void uart1_input(u8 data);
  void uart2_input(u8 data);

  // PS/2 callbacks
  void ps2_keyboard_input(u8 scancode);
  void ps2_mouse_input(u8 data);

  // RTC
  void update_rtc_time();

private:
  std::array<u8, 0x40000> regs_ = {}; // 256KB register space

  // UART 1 state
  u8 uart1_dll_ = 0, uart1_dlm_ = 0;
  u8 uart1_lcr_ = 0;
  u8 uart1_ier_ = 0;
  u8 uart1_fcr_ = 0;
  u8 uart1_mcr_ = 0;
  std::queue<u8> uart1_rx_fifo_;
  std::queue<u8> uart1_tx_fifo_;
  UartCallback uart1_output_cb_;

  // UART 2 state
  u8 uart2_dll_ = 0, uart2_dlm_ = 0;
  u8 uart2_lcr_ = 0;
  u8 uart2_ier_ = 0;
  u8 uart2_fcr_ = 0;
  u8 uart2_mcr_ = 0;
  std::queue<u8> uart2_rx_fifo_;
  std::queue<u8> uart2_tx_fifo_;
  UartCallback uart2_output_cb_;

  // RTC state
  std::array<u8, 128> rtc_regs_ = {};
  u64 rtc_base_time_ = 0; // Unix timestamp at reset

  // PS/2 state
  std::queue<u8> ps2_kbd_fifo_;
  std::queue<u8> ps2_mouse_fifo_;
  u8 ps2_kbd_status_ = 0;
  u8 ps2_mouse_status_ = 0;
};

} // namespace o2emu::devices