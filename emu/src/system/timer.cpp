/**
 * @file timer.cpp
 * @brief System timer implementation
 */

#include <o2emu/logging/logger.h>
#include <o2emu/system/timer.h>

namespace o2emu::system {

Timer::Timer()
    : frequency_(133000000) // 133 MHz system clock
      ,
      counter_(0), compare_(0), enabled_(false), periodic_(false),
      interrupt_enabled_(false), interrupt_pending_(false) {}

Timer::~Timer() = default;

void Timer::reset() {
  counter_ = 0;
  compare_ = 0;
  enabled_ = false;
  periodic_ = false;
  interrupt_enabled_ = false;
  interrupt_pending_ = false;
}

void Timer::set_frequency(u64 hz) { frequency_ = hz; }

u64 Timer::frequency() const { return frequency_; }

void Timer::set_compare(u64 value) { compare_ = value; }

u64 Timer::compare() const { return compare_; }

void Timer::set_enabled(bool enabled) {
  enabled_ = enabled;
  if (enabled) {
    counter_ = 0;
  }
}

bool Timer::enabled() const { return enabled_; }

void Timer::set_periodic(bool periodic) { periodic_ = periodic; }

bool Timer::periodic() const { return periodic_; }

void Timer::set_interrupt_enabled(bool enabled) {
  interrupt_enabled_ = enabled;
}

bool Timer::interrupt_enabled() const { return interrupt_enabled_; }

bool Timer::interrupt_pending() const { return interrupt_pending_; }

void Timer::clear_interrupt() { interrupt_pending_ = false; }

u64 Timer::counter() const { return counter_; }

void Timer::tick(u64 cycles) {
  if (!enabled_)
    return;

  counter_ += cycles;

  if (counter_ >= compare_) {
    interrupt_pending_ = true;

    if (periodic_) {
      counter_ = 0;
    } else {
      enabled_ = false;
    }
  }
}

} // namespace o2emu::system