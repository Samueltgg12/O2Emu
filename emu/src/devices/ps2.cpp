/**
 * @file ps2.cpp
 * @brief PS/2 keyboard and mouse controller implementation
 */

#include <cstring>
#include <iomanip>
#include <o2emu/devices/ps2.h>
#include <o2emu/logging/logger.h>
#include <sstream>

namespace o2emu::devices {

static std::string to_hex(u32 value) {
  std::stringstream ss;
  ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
     << value;
  return ss.str();
}

PS2::PS2(u32 base_addr, u32 kbd_irq, u32 mouse_irq)
    : Device("PS/2", base_addr, 0x10), kbd_irq_(kbd_irq), mouse_irq_(mouse_irq),
      kbd_buffer_(), mouse_buffer_() {
  reset();
}

PS2::~PS2() = default;

void PS2::reset() {
  Device::reset();
  std::memset(regs_.data(), 0, regs_.size());

  // PS/2 controller register defaults (8042 compatible)
  // Data port: 0x00
  regs_[REG_DATA] = 0x00;

  // Status/Command: 0x04
  regs_[REG_STATUS] = 0x00;
  regs_[REG_COMMAND] = 0x00;

  // Keyboard/mouse specific
  kbd_buffer_.clear();
  mouse_buffer_.clear();
  kbd_enabled_ = true;
  mouse_enabled_ = true;
}

u32 PS2::read_reg(u32 offset) {
  switch (offset) {
  case 0x00: // Data port
    // Return keyboard data if available, else mouse data
    if (!kbd_buffer_.empty()) {
      u8 data = kbd_buffer_.front();
      kbd_buffer_.pop_front();
      regs_[REG_STATUS] &= ~0x01; // Clear output buffer full
      return data;
    } else if (!mouse_buffer_.empty()) {
      u8 data = mouse_buffer_.front();
      mouse_buffer_.pop_front();
      regs_[REG_STATUS] &= ~0x01; // Clear output buffer full
      return data | 0x80;         // Mouse data flag
    }
    return 0x00;

  case 0x04: // Status register
    return regs_[REG_STATUS];

  default:
    return 0;
  }
}

void PS2::write_reg(u32 offset, u32 value) {
  switch (offset) {
  case 0x00: // Data port (to keyboard/mouse)
    // Host sending data to device
    if (regs_[REG_STATUS] & 0x20) { // Mouse data expected
      handle_mouse_command(value);
    } else {
      handle_keyboard_command(value);
    }
    break;

  case 0x04: // Command register
    handle_controller_command(value);
    break;

  default:
    break;
  }
}

void PS2::handle_controller_command(u8 cmd) {
  switch (cmd) {
  case 0x20:                   // Read command byte
    regs_[REG_DATA] = 0x00;    // Default command byte
    regs_[REG_STATUS] |= 0x01; // Output buffer full
    break;

  case 0x60: // Write command byte
    // Next byte written to data port is command byte
    regs_[REG_STATUS] |= 0x02; // Input buffer full
    break;

  case 0xA7: // Disable mouse
    mouse_enabled_ = false;
    break;

  case 0xA8: // Enable mouse
    mouse_enabled_ = true;
    break;

  case 0xA9:                // Test mouse interface
    regs_[REG_DATA] = 0x00; // OK
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xAA:                // Test controller
    regs_[REG_DATA] = 0x55; // OK
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xAB:                // Test keyboard interface
    regs_[REG_DATA] = 0x00; // OK
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xAD: // Disable keyboard
    kbd_enabled_ = false;
    break;

  case 0xAE: // Enable keyboard
    kbd_enabled_ = true;
    break;

  case 0xC0: // Read input port
    regs_[REG_DATA] = 0x00;
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xD0: // Read output port
    regs_[REG_DATA] = 0x00;
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xD1: // Write output port
    regs_[REG_STATUS] |= 0x02;
    break;

  default:
    O2EMU_LOG_DEBUG("PS/2 unknown controller command: 0x" + to_hex((int)cmd));
    break;
  }
}

void PS2::handle_keyboard_command(u8 cmd) {
  switch (cmd) {
  case 0xED: // Set LEDs
    // Next byte is LED state
    regs_[REG_STATUS] |= 0x02;
    break;

  case 0xEE: // Echo
    kbd_buffer_.push_back(0xEE);
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xF0: // Set scan code set
    regs_[REG_STATUS] |= 0x02;
    break;

  case 0xF2:                     // Identify keyboard
    kbd_buffer_.push_back(0xFA); // ACK
    kbd_buffer_.push_back(0xAB);
    kbd_buffer_.push_back(0x83);
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xF3: // Set typematic rate
    regs_[REG_STATUS] |= 0x02;
    break;

  case 0xF4:                     // Enable scanning
    kbd_buffer_.push_back(0xFA); // ACK
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xF5:                     // Disable scanning
    kbd_buffer_.push_back(0xFA); // ACK
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xF6:                     // Set default
    kbd_buffer_.push_back(0xFA); // ACK
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xFF:                     // Reset
    kbd_buffer_.push_back(0xFA); // ACK
    kbd_buffer_.push_back(0xAA); // BAT passed
    regs_[REG_STATUS] |= 0x01;
    break;

  default:
    O2EMU_LOG_DEBUG("PS/2 unknown keyboard command: 0x" + to_hex((int)cmd));
    break;
  }
}

void PS2::handle_mouse_command(u8 cmd) {
  switch (cmd) {
  case 0xE6:                       // Set scaling 1:1
    mouse_buffer_.push_back(0xFA); // ACK
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xE7:                       // Set scaling 2:1
    mouse_buffer_.push_back(0xFA); // ACK
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xE8: // Set resolution
    regs_[REG_STATUS] |= 0x02;
    break;

  case 0xE9:                       // Status request
    mouse_buffer_.push_back(0xFA); // ACK
    mouse_buffer_.push_back(0x00); // Status
    mouse_buffer_.push_back(0x00); // Resolution
    mouse_buffer_.push_back(0x00); // Sample rate
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xEA:                       // Set stream mode
    mouse_buffer_.push_back(0xFA); // ACK
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xEB:                       // Read data
    mouse_buffer_.push_back(0xFA); // ACK
    mouse_buffer_.push_back(0x00); // Buttons
    mouse_buffer_.push_back(0x00); // X
    mouse_buffer_.push_back(0x00); // Y
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xEC:                       // Reset wrap mode
    mouse_buffer_.push_back(0xFA); // ACK
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xEE:                       // Set wrap mode
    mouse_buffer_.push_back(0xFA); // ACK
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xF0:                       // Set remote mode
    mouse_buffer_.push_back(0xFA); // ACK
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xF2:                       // Identify mouse
    mouse_buffer_.push_back(0xFA); // ACK
    mouse_buffer_.push_back(0x00); // Standard mouse
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xF3: // Set sample rate
    regs_[REG_STATUS] |= 0x02;
    break;

  case 0xF4:                       // Enable data reporting
    mouse_buffer_.push_back(0xFA); // ACK
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xF5:                       // Disable data reporting
    mouse_buffer_.push_back(0xFA); // ACK
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xF6:                       // Set defaults
    mouse_buffer_.push_back(0xFA); // ACK
    regs_[REG_STATUS] |= 0x01;
    break;

  case 0xFF:                       // Reset
    mouse_buffer_.push_back(0xFA); // ACK
    mouse_buffer_.push_back(0xAA); // BAT passed
    mouse_buffer_.push_back(0x00); // ID
    regs_[REG_STATUS] |= 0x01;
    break;

  default:
    O2EMU_LOG_DEBUG("PS/2 unknown mouse command: 0x" + to_hex((int)cmd));
    break;
  }
}

bool PS2::read(u32 offset, [[maybe_unused]] u32 size, u32 &value) {
  if (offset < 8) {
    value = read_reg(offset);
    return true;
  }
  return false;
}

bool PS2::write(u32 offset, [[maybe_unused]] u32 size, u32 value) {
  if (offset < 8) {
    write_reg(offset, value);
    return true;
  }
  return false;
}

void PS2::tick([[maybe_unused]] u64 cycles) {
  // PS/2 doesn't need periodic updates in this simple implementation
}

void PS2::push_keyboard_scancode(u8 scancode) {
  if (kbd_enabled_) {
    kbd_buffer_.push_back(scancode);
    regs_[REG_STATUS] |= 0x01;  // Output buffer full
    regs_[REG_STATUS] &= ~0x20; // Keyboard data
  }
}

void PS2::push_mouse_data(u8 buttons, i8 dx, i8 dy) {
  if (mouse_enabled_) {
    mouse_buffer_.push_back(0x08 |
                            (buttons & 0x07)); // Buttons + overflow flags
    mouse_buffer_.push_back(static_cast<u8>(dx));
    mouse_buffer_.push_back(static_cast<u8>(dy));
    regs_[REG_STATUS] |= 0x01; // Output buffer full
    regs_[REG_STATUS] |= 0x20; // Mouse data
  }
}

u32 PS2::interrupt_status() const {
  u32 status = 0;
  if (!kbd_buffer_.empty()) {
    status |= (1 << kbd_irq_);
  }
  if (!mouse_buffer_.empty()) {
    status |= (1 << mouse_irq_);
  }
  return status;
}

} // namespace o2emu::devices