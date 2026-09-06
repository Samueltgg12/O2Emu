/**
 * @file interrupts.cpp
 * @brief Interrupt controller implementation
 */

#include <o2emu/system/interrupts.h>

namespace o2emu::system {

InterruptController::InterruptController() = default;

void InterruptController::raise(Line line) {
  pending_ |= (1u << static_cast<u32>(line));
}

void InterruptController::clear(Line line) {
  pending_ &= ~(1u << static_cast<u32>(line));
}

bool InterruptController::is_pending(Line line) const {
  return (pending_ & (1u << static_cast<u32>(line))) != 0;
}

u32 InterruptController::pending_mask() const { return pending_ & mask_; }

void InterruptController::set_handler(Line line, Handler handler) {
  handlers_[static_cast<u32>(line)] = std::move(handler);
}

void InterruptController::process() {
  u32 masked = pending_ & mask_;
  while (masked) {
    int line_idx = 31 - __builtin_clz(masked);
    masked &= ~(1u << line_idx);

    Line line = static_cast<Line>(line_idx);
    if (handlers_[line_idx]) {
      handlers_[line_idx](line);
    }
  }
}

void InterruptController::reset() {
  pending_ = 0;
  mask_ = 0;
  for (auto &handler : handlers_) {
    handler = nullptr;
  }
}

} // namespace o2emu::system