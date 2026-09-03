#pragma once

/**
 * @file interrupts.h
 * @brief Interrupt controller
 */

#include <array>
#include <functional>
#include <o2emu/o2emu.h>

namespace o2emu::system {

class InterruptController {
public:
  InterruptController();
  ~InterruptController() = default;

  // IP32 interrupt lines (from CRIME interrupt controller)
  enum Line : uint32_t {
    // CRIME interrupts (0-31)
    INT_PCI_ERROR = 0,
    INT_PCI_SERR = 1,
    INT_PCI_PERR = 2,
    INT_MEMORY_ECC = 3,
    INT_MEMORY_REFRESH = 4,
    INT_TIMER_0 = 5,
    INT_TIMER_1 = 6,
    INT_UART_1 = 7,
    INT_UART_2 = 8,
    INT_RTC = 9,
    INT_PS2_KEYBOARD = 10,
    INT_PS2_MOUSE = 11,
    INT_ETHERNET = 12,
    INT_AUDIO = 13,
    INT_SCSI_0 = 14,
    INT_SCSI_1 = 15,
    INT_GRAPHICS = 16,
    INT_VERTICAL_RETRACE = 17,

    // CPU interrupts (mapped to CP0)
    CPU_INT_0 = 0, // Maps to CP0 INT0
    CPU_INT_1 = 1,
    CPU_INT_2 = 2,
    CPU_INT_3 = 3,
    CPU_INT_4 = 4,
    CPU_INT_5 = 5,
    CPU_INT_6 = 6,
    CPU_INT_7 = 7,
  };

  // Raise/clear interrupt
  void raise(Line line);
  void clear(Line line);
  bool is_pending(Line line) const;

  // Get pending interrupts as bitmask
  u32 pending_mask() const;

  // Set interrupt handler callback
  using Handler = std::function<void(Line)>;
  void set_handler(Line line, Handler handler);

  // Process pending interrupts (call handlers)
  void process();

  // Reset
  void reset();

private:
  u32 pending_ = 0;
  u32 mask_ = 0xFFFFFFFF;
  std::array<Handler, 32> handlers_;
};

} // namespace o2emu::system