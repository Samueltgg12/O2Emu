/**
 * @file uart.h
 * @brief UART (16550 compatible) interface
 *
 * Based on NS16550A datasheet and IRIX serial driver
 * O2 uses 16550-compatible UARTs for serial ports
 */

#pragma once

#include <array>
#include <deque>
#include <o2emu/devices/device.h>
#include <o2emu/o2emu.h>

namespace o2emu::devices {

class UART : public Device {
public:
  UART(u32 base_addr, u32 irq);
  ~UART() override;

  // 16550 register offsets
  enum Register : uint32_t {
    REG_RBR = 0x00, // Receiver Buffer Register (read, DLAB=0)
    REG_THR = 0x00, // Transmitter Holding Register (write, DLAB=0)
    REG_DLL = 0x00, // Divisor Latch Low (DLAB=1)
    REG_IER = 0x01, // Interrupt Enable Register (DLAB=0)
    REG_DLM = 0x01, // Divisor Latch High (DLAB=1)
    REG_IIR = 0x02, // Interrupt Identification Register (read)
    REG_FCR = 0x02, // FIFO Control Register (write)
    REG_LCR = 0x03, // Line Control Register
    REG_MCR = 0x04, // Modem Control Register
    REG_LSR = 0x05, // Line Status Register
    REG_MSR = 0x06, // Modem Status Register
    REG_SCR = 0x07, // Scratch Register
  };

  // LCR bits
  enum LcrBit : uint8_t {
    LCR_DLAB = 0x80, // Divisor Latch Access Bit
    LCR_SB = 0x40,   // Set Break
    LCR_PEN = 0x08,  // Parity Enable
    LCR_EPS = 0x10,  // Even Parity Select
    LCR_STB = 0x04,  // Stop Bits
    LCR_WLS = 0x03,  // Word Length Select
  };

  // IER bits
  enum IerBit : uint8_t {
    IER_RDA = 0x01,  // Received Data Available
    IER_THRE = 0x02, // Transmitter Holding Register Empty
    IER_RLS = 0x04,  // Receiver Line Status
    IER_MS = 0x08,   // Modem Status
  };

  // IIR bits
  enum IirBit : uint8_t {
    IIR_NOPEND = 0x01, // No interrupt pending
    IIR_THRE = 0x02,   // THRE interrupt
    IIR_RDA = 0x04,    // RDA interrupt
    IIR_RLS = 0x06,    // RLS interrupt
    IIR_MS = 0x00,     // MS interrupt
    IIR_FIFO = 0xC0,   // FIFO enabled
  };

  // LSR bits
  enum LsrBit : uint8_t {
    LSR_DR = 0x01,   // Data Ready
    LSR_OE = 0x02,   // Overrun Error
    LSR_PE = 0x04,   // Parity Error
    LSR_FE = 0x08,   // Framing Error
    LSR_BI = 0x10,   // Break Interrupt
    LSR_THRE = 0x20, // Transmitter Holding Register Empty
    LSR_TEMT = 0x40, // Transmitter Empty
    LSR_FERR = 0x80, // FIFO Error
  };

  // MCR bits
  enum McrBit : uint8_t {
    MCR_DTR = 0x01,  // Data Terminal Ready
    MCR_RTS = 0x02,  // Request to Send
    MCR_OUT1 = 0x04, // Output 1
    MCR_OUT2 = 0x08, // Output 2 (interrupt enable)
    MCR_LOOP = 0x10, // Loopback mode
  };

  // MSR bits
  enum MsrBit : uint8_t {
    MSR_DCTS = 0x01, // Delta CTS
    MSR_DDSR = 0x02, // Delta DSR
    MSR_TERI = 0x04, // Trailing Edge RI
    MSR_DDCD = 0x08, // Delta DCD
    MSR_CTS = 0x10,  // Clear to Send
    MSR_DSR = 0x20,  // Data Set Ready
    MSR_RI = 0x40,   // Ring Indicator
    MSR_DCD = 0x80,  // Data Carrier Detect
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

  // UART-specific
  void push_rx_char(u8 ch);
  u32 interrupt_status() const;

  // Generic read/write for bus interface
  bool read(u32 offset, u32 size, u32 &value);
  bool write(u32 offset, u32 size, u32 value);

private:
  u32 irq_;
  u16 divisor_;
  std::array<u8, 8> regs_ = {};
  std::deque<u8> rx_buffer_;
  std::deque<u8> tx_buffer_;

  u32 read_reg(u32 offset);
  void write_reg(u32 offset, u32 value);
  void update_baud_rate();
  void update_interrupts();
};

} // namespace o2emu::devices