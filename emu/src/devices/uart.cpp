/**
 * @file uart.cpp
 * @brief UART (16550 compatible) implementation
 */

#include <cstring>
#include <o2emu/devices/uart.h>
#include <o2emu/logging/logger.h>

namespace o2emu::devices {

UART::UART(u32 base_addr, u32 irq)
    : Device("UART", base_addr, 0x8), irq_(irq), divisor_(1), rx_buffer_(),
      tx_buffer_() {
  reset();
}

UART::~UART() = default;

void UART::reset() {
  Device::reset();
  std::memset(regs_.data(), 0, regs_.size());

  // 16550 register defaults
  // RBR/THR/DLL: 0x00
  regs_[REG_RBR] = 0x00;

  // IER/DLM: 0x01
  regs_[REG_IER] = 0x00;

  // IIR/FCR: 0x02
  regs_[REG_IIR] = 0x01; // No interrupt pending

  // LCR: 0x03
  regs_[REG_LCR] = 0x00;

  // MCR: 0x04
  regs_[REG_MCR] = 0x00;

  // LSR: 0x05
  regs_[REG_LSR] = 0x60; // THRE=1, TEMT=1

  // MSR: 0x06
  regs_[REG_MSR] = 0x00;

  // SCR: 0x07
  regs_[REG_SCR] = 0x00;

  divisor_ = 1;
  rx_buffer_.clear();
  tx_buffer_.clear();
}

u32 UART::read_reg(u32 offset) {
  if (offset >= 8)
    return 0;

  bool dlab = (regs_[REG_LCR] & 0x80) != 0;

  switch (offset) {
  case 0x00:
    if (dlab) {
      return divisor_ & 0xFF; // DLL
    }
    // RBR - Receiver Buffer Register
    regs_[REG_LSR] &= ~0x01; // Clear DR (Data Ready)
    if (!rx_buffer_.empty()) {
      u8 data = rx_buffer_.front();
      rx_buffer_.pop_front();
      return data;
    }
    return 0x00;

  case 0x01:
    if (dlab) {
      return (divisor_ >> 8) & 0xFF; // DLM
    }
    return regs_[REG_IER]; // IER

  case 0x02:
    // IIR - Interrupt Identification Register
    return regs_[REG_IIR];

  case 0x03:
    return regs_[REG_LCR];

  case 0x04:
    return regs_[REG_MCR];

  case 0x05:
    return regs_[REG_LSR];

  case 0x06:
    return regs_[REG_MSR];

  case 0x07:
    return regs_[REG_SCR];

  default:
    return 0;
  }
}

void UART::write_reg(u32 offset, u32 value) {
  if (offset >= 8)
    return;

  bool dlab = (regs_[REG_LCR] & 0x80) != 0;

  switch (offset) {
  case 0x00:
    if (dlab) {
      divisor_ = (divisor_ & 0xFF00) | (value & 0xFF);
      update_baud_rate();
    } else {
      // THR - Transmitter Holding Register
      tx_buffer_.push_back(static_cast<u8>(value));
      regs_[REG_LSR] &= ~0x20; // Clear THRE
      regs_[REG_LSR] |= 0x40;  // Set TEMT

      // Simulate immediate transmission
      regs_[REG_LSR] |= 0x20; // Set THRE
      regs_[REG_IIR] = 0x02;  // THRE interrupt
    }
    break;

  case 0x01:
    if (dlab) {
      divisor_ = (divisor_ & 0x00FF) | ((value & 0xFF) << 8);
      update_baud_rate();
    } else {
      regs_[REG_IER] = value & 0x0F;
      update_interrupts();
    }
    break;

  case 0x02:
    // FCR - FIFO Control Register
    regs_[REG_FCR] = value;
    break;

  case 0x03:
    regs_[REG_LCR] = value;
    break;

  case 0x04:
    regs_[REG_MCR] = value;
    break;

  case 0x05:
    // LSR is read-only
    break;

  case 0x06:
    // MSR is read-only
    break;

  case 0x07:
    regs_[REG_SCR] = value;
    break;
  }
}

void UART::update_baud_rate() {
  // Baud rate = 115200 / divisor
  // For simulation, we don't actually use this
  O2EMU_LOG_DEBUG("UART baud rate divisor: " << static_cast<u32>(divisor_));
}

void UART::update_interrupts() {
  u8 ier = regs_[REG_IER];
  u8 iir = 0x01; // No interrupt

  if ((ier & 0x02) && (regs_[REG_LSR] & 0x20)) { // THRE interrupt
    iir = 0x02;
  } else if ((ier & 0x01) && (regs_[REG_LSR] & 0x01)) { // RDA interrupt
    iir = 0x04;
  } else if ((ier & 0x04) && (regs_[REG_LSR] & 0x1E)) { // LSR interrupt
    iir = 0x06;
  } else if ((ier & 0x08) && (regs_[REG_MSR] & 0x0F)) { // MSR interrupt
    iir = 0x00;
  }

  regs_[REG_IIR] = iir;
}

bool UART::read(u32 offset, [[maybe_unused]] u32 size, u32 &value) {
  if (offset < 8) {
    value = read_reg(offset);
    return true;
  }
  return false;
}

bool UART::write(u32 offset, [[maybe_unused]] u32 size, u32 value) {
  if (offset < 8) {
    write_reg(offset, value);
    return true;
  }
  return false;
}

void UART::tick([[maybe_unused]] u64 cycles) {
  // Simulate character reception (for testing)
  // In a real implementation, this would be connected to a terminal
}

void UART::push_rx_char(u8 ch) {
  rx_buffer_.push_back(ch);
  regs_[REG_LSR] |= 0x01; // Set DR (Data Ready)

  if (regs_[REG_IER] & 0x01) { // RDA interrupt enabled
    regs_[REG_IIR] = 0x04;     // RDA interrupt
  }
}

u32 UART::interrupt_status() const {
  u8 iir = regs_[REG_IIR];
  if ((iir & 0x01) == 0) { // Interrupt pending
    return 1 << irq_;
  }
  return 0;
}

} // namespace o2emu::devices