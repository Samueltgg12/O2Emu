#pragma once

/**
 * @file timer.h
 * @brief System timer
 */

#include <functional>
#include <o2emu/o2emu.h>

namespace o2emu::system {

class Timer {
public:
  Timer();
  ~Timer() = default;

  // Timer IDs
  enum ID : uint32_t {
    TIMER_0 = 0,   // CRIME timer 0
    TIMER_1 = 1,   // CRIME timer 1
    TIMER_CPU = 2, // CP0 COUNT/COMPARE
  };

  // Initialize timer
  void init(ID id, u32 frequency_hz);

  // Start/stop timer
  void start(ID id);
  void stop(ID id);
  bool running(ID id) const;

  // Set compare value (for CP0-style timer)
  void set_compare(ID id, u32 compare);
  u32 compare(ID id) const;

  // Get current count
  u32 count(ID id) const;

  // Tick timer (call periodically)
  void tick(ID id, u64 cycles);

  // Check if interrupt should fire
  bool interrupt_pending(ID id) const;

  // Clear interrupt
  void clear_interrupt(ID id);

  // Set callback
  using Callback = std::function<void(ID)>;
  void set_callback(ID id, Callback cb);

  // Reset
  void reset();

private:
  struct TimerState {
    u64 frequency = 0;
    u64 counter = 0;
    u32 compare = 0;
    bool running = false;
    bool interrupt_pending = false;
    Callback callback;
  };

  std::array<TimerState, 3> timers_;
};

} // namespace o2emu::system