#pragma once

/**
 * @file mace_isa.h
 * @brief MACE ISA Bridge (ISA bus controller, DMA, timers, UARTs, PS/2,
 * Parallel)
 *
 * Offset from MACE base: 0x380000 (MACE_ISA_EXTERNAL_OFFSET)
 * Based on Linux kernel drivers and PROM decompiled definitions
 */

#include <array>
#include <functional>
#include <o2emu/devices/device.h>
#include <o2emu/o2emu.h>
#include <queue>

namespace o2emu::devices {

class MACE;

class MACEISA : public Device {
public:
  explicit MACEISA(MACE &mace);
  ~MACEISA() override = default;

  // ISA register indices (32-bit word indices, not byte offsets)
  enum Register : uint32_t {
    // ISA Control/Status
    REG_CTRL = 0x0000 / 4,   // 0x0000
    REG_STATUS = 0x0008 / 4, // 0x0008

    // ISA Address Space
    REG_IO_BASE = 0x0010 / 4,  // 0x0010
    REG_MEM_BASE = 0x0018 / 4, // 0x0018

    // DMA Controller
    REG_DMA_CTRL = 0x0100 / 4,   // 0x0100
    REG_DMA_ADDR = 0x0108 / 4,   // 0x0108
    REG_DMA_COUNT = 0x0110 / 4,  // 0x0110
    REG_DMA_STATUS = 0x0118 / 4, // 0x0118

    // Timer
    REG_TIMER_CTRL = 0x0200 / 4,  // 0x0200
    REG_TIMER_COUNT = 0x0208 / 4, // 0x0208

    // Interrupt Controller
    REG_INT_STATUS = 0x0300 / 4, // 0x0300
    REG_INT_MASK = 0x0308 / 4,   // 0x0308
    REG_INT_CLEAR = 0x0310 / 4,  // 0x0310

    // PS/2 Keyboard
    REG_PS2_KBD_DATA = 0x0400 / 4, // 0x0400
    REG_PS2_KBD_CTRL = 0x0408 / 4, // 0x0408

    // PS/2 Mouse
    REG_PS2_MOUSE_DATA = 0x0410 / 4, // 0x0410
    REG_PS2_MOUSE_CTRL = 0x0418 / 4, // 0x0418

    // Serial ports (2x 16550 compatible) - 8 registers each at 0x100 spacing
    REG_UART_BASE = 0x0500 / 4, // 0x0500

    // Parallel port
    REG_PAR_DATA = 0x0600 / 4,   // 0x0600
    REG_PAR_STATUS = 0x0608 / 4, // 0x0608
    REG_PAR_CTRL = 0x0610 / 4,   // 0x0610

    // Total register count (must be last)
    REG_COUNT = 0x10000 / 4, // 64K 32-bit registers = 256KB
  };

  // Device interface
  u32 read32(u32 offset) override;
  u16 read16(u32 offset) override;
  u8 read8(u32 offset) override;

  void write32(u32 offset, u32 value) override;
  void write16(u32 offset, u16 value) override;
  void write8(u32 offset, u8 value) override;

  // Register-level access (used by MACE)
  u32 read_reg(u32 offset);
  void write_reg(u32 offset, u32 value);

  // DMA
  void start_dma();

  // Config space access (called by MACE)
  void config_write(u32 reg, u32 value);

  // Generic read/write for device interface
  bool read(u32 offset, [[maybe_unused]] u32 size, u32 &value);
  bool write(u32 offset, [[maybe_unused]] u32 size, u32 value);

  void reset() override;
  void tick(u64 cycles) override;

  u32 interrupt_status() const override;
  void clear_interrupt(u32 mask) override;

  // UART callbacks
  using UartCallback = std::function<void(u8 data)>;
  void set_uart1_output(UartCallback cb) { uart1_output_cb_ = std::move(cb); }
  void set_uart2_output(UartCallback cb) { uart2_output_cb_ = std::move(cb); }
  void uart1_input(u8 data);
  void uart2_input(u8 data);

  // PS/2 callbacks
  void ps2_keyboard_input(u8 scancode);
  void ps2_mouse_input(u8 data);

private:
  MACE &mace_;
  std::array<u32, REG_COUNT> regs_ = {};

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

  // PS/2 state
  std::queue<u8> ps2_kbd_fifo_;
  std::queue<u8> ps2_mouse_fifo_;
  u8 ps2_kbd_status_ = 0;
  u8 ps2_mouse_status_ = 0;
};

} // namespace o2emu::devices