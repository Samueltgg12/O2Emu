/**
 * @file ps2.h
 * @brief PS/2 keyboard and mouse controller interface
 *
 * Based on 8042 keyboard controller and IRIX PS/2 driver
 * O2 uses PS/2 for keyboard and mouse
 */

#pragma once

#include <array>
#include <deque>
#include <o2emu/devices/device.h>
#include <o2emu/o2emu.h>

namespace o2emu::devices {

class PS2 : public Device {
public:
  PS2(u32 base_addr, u32 kbd_irq, u32 mouse_irq);
  ~PS2() override;

  // PS/2 controller register offsets (8042 compatible)
  enum Register : uint32_t {
    REG_DATA = 0x00,    // Data port (read/write)
    REG_STATUS = 0x04,  // Status register (read)
    REG_COMMAND = 0x04, // Command register (write)
  };

  // Status register bits
  enum StatusBit : uint8_t {
    STATUS_OBF = 0x01,     // Output Buffer Full
    STATUS_IBF = 0x02,     // Input Buffer Full
    STATUS_SYS = 0x04,     // System flag
    STATUS_CMD = 0x08,     // Command/Data (0=data, 1=command)
    STATUS_INH = 0x10,     // Keyboard inhibit
    STATUS_MOUSE = 0x20,   // Mouse data (1=mouse, 0=keyboard)
    STATUS_TIMEOUT = 0x40, // Timeout
    STATUS_PARITY = 0x80,  // Parity error
  };

  // Controller commands
  enum ControllerCmd : uint8_t {
    CMD_READ_CMD_BYTE = 0x20,
    CMD_WRITE_CMD_BYTE = 0x60,
    CMD_DISABLE_MOUSE = 0xA7,
    CMD_ENABLE_MOUSE = 0xA8,
    CMD_TEST_MOUSE = 0xA9,
    CMD_TEST_CONTROLLER = 0xAA,
    CMD_TEST_KEYBOARD = 0xAB,
    CMD_DISABLE_KEYBOARD = 0xAD,
    CMD_ENABLE_KEYBOARD = 0xAE,
    CMD_READ_INPUT_PORT = 0xC0,
    CMD_READ_OUTPUT_PORT = 0xD0,
    CMD_WRITE_OUTPUT_PORT = 0xD1,
  };

  // Keyboard commands
  enum KeyboardCmd : uint8_t {
    KBD_CMD_SET_LEDS = 0xED,
    KBD_CMD_ECHO = 0xEE,
    KBD_CMD_SET_SCANCODE = 0xF0,
    KBD_CMD_IDENTIFY = 0xF2,
    KBD_CMD_SET_TYPEMATIC = 0xF3,
    KBD_CMD_ENABLE = 0xF4,
    KBD_CMD_DISABLE = 0xF5,
    KBD_CMD_SET_DEFAULT = 0xF6,
    KBD_CMD_RESET = 0xFF,
  };

  // Mouse commands
  enum MouseCmd : uint8_t {
    MOUSE_CMD_SCALING_1_1 = 0xE6,
    MOUSE_CMD_SCALING_2_1 = 0xE7,
    MOUSE_CMD_SET_RESOLUTION = 0xE8,
    MOUSE_CMD_STATUS = 0xE9,
    MOUSE_CMD_STREAM = 0xEA,
    MOUSE_CMD_READ_DATA = 0xEB,
    MOUSE_CMD_RESET_WRAP = 0xEC,
    MOUSE_CMD_SET_WRAP = 0xEE,
    MOUSE_CMD_REMOTE = 0xF0,
    MOUSE_CMD_IDENTIFY = 0xF2,
    MOUSE_CMD_SET_SAMPLE_RATE = 0xF3,
    MOUSE_CMD_ENABLE = 0xF4,
    MOUSE_CMD_DISABLE = 0xF5,
    MOUSE_CMD_SET_DEFAULTS = 0xF6,
    MOUSE_CMD_RESET = 0xFF,
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

  // PS/2-specific
  void push_keyboard_scancode(u8 scancode);
  void push_mouse_data(u8 buttons, i8 dx, i8 dy);
  u32 interrupt_status() const;

  // Generic read/write for bus interface
  bool read(u32 offset, u32 size, u32 &value);
  bool write(u32 offset, u32 size, u32 value);

private:
  u32 kbd_irq_;
  u32 mouse_irq_;
  std::array<u8, 8> regs_ = {};
  std::deque<u8> kbd_buffer_;
  std::deque<u8> mouse_buffer_;
  bool kbd_enabled_ = true;
  bool mouse_enabled_ = true;

  u32 read_reg(u32 offset);
  void write_reg(u32 offset, u32 value);
  void handle_controller_command(u8 cmd);
  void handle_keyboard_command(u8 cmd);
  void handle_mouse_command(u8 cmd);
};

} // namespace o2emu::devices