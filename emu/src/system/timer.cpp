/**
 * @file timer.cpp
 * @brief System timer implementation
 */

#include <o2emu/logging/logger.h>
#include <o2emu/system/timer.h>

namespace o2emu::system {

Timer::Timer() {
  // Initialize all timers to default state
  for (auto &timer : timers_) {
    timer.frequency = 0;
    timer.counter = 0;
    timer.compare = 0;
    timer.running = false;
    timer.interrupt_pending = false;
    timer.callback = nullptr;
  }
}

void Timer::init(ID id, u32 frequency_hz) {
  if (id < timers_.size()) {
    timers_[id].frequency = frequency_hz;
    timers_[id].counter = 0;
    timers_[id].compare = 0;
    timers_[id].running = false;
    timers_[id].interrupt_pending = false;
  }
}

void Timer::start(ID id) {
  if (id < timers_.size()) {
    timers_[id].running = true;
    timers_[id].counter = 0;
  }
}

void Timer::stop(ID id) {
  if (id < timers_.size()) {
    timers_[id].running = false;
  }
}

bool Timer::running(ID id) const {
  if (id < timers_.size()) {
    return timers_[id].running;
  }
  return false;
}

void Timer::set_compare(ID id, u32 compare) {
  if (id < timers_.size()) {
    timers_[id].compare = compare;
  }
}

u32 Timer::compare(ID id) const {
  if (id < timers_.size()) {
    return timers_[id].compare;
  }
  return 0;
}

u32 Timer::count(ID id) const {
  if (id < timers_.size()) {
    return static_cast<u32>(timers_[id].counter);
  }
  return 0;
}

void Timer::tick(ID id, u64 cycles) {
  if (id >= timers_.size())
    return;

  auto &timer = timers_[id];
  if (!timer.running || timer.frequency == 0)
    return;

  timer.counter += cycles;

  if (timer.counter >= timer.compare) {
    timer.interrupt_pending = true;
    if (timer.callback) {
      timer.callback(id);
    }
  }
}

bool Timer::interrupt_pending(ID id) const {
  if (id < timers_.size()) {
    return timers_[id].interrupt_pending;
  }
  return false;
}

void Timer::clear_interrupt(ID id) {
  if (id < timers_.size()) {
    timers_[id].interrupt_pending = false;
  }
}

void Timer::set_callback(ID id, Callback cb) {
  if (id < timers_.size()) {
    timers_[id].callback = std::move(cb);
  }
}

void Timer::reset() {
  for (auto &timer : timers_) {
    timer.frequency = 0;
    timer.counter = 0;
    timer.compare = 0;
    timer.running = false;
    timer.interrupt_pending = false;
    timer.callback = nullptr;
  }
}

} // namespace o2emu::system