/**
 * @file interrupts.cpp
 * @brief Interrupt controller implementation
 */

#include <cstring>
#include <o2emu/logging/logger.h>
#include <o2emu/system/interrupts.h>

namespace o2emu::system {

InterruptController::InterruptController() : pending_(0), mask_(0), level_(0) {
  reset();
}

InterruptController::~InterruptController() = default;

void InterruptController::reset() {
  pending_ = 0;
  mask_ = 0;
  level_ = 0;
  std::memset(handlers_, 0, sizeof(handlers_));
}

void InterruptController::set_pending(u32 irq) {
  if (irq < 32) {
    pending_ |= (1u << irq);
  }
}

void InterruptController::clear_pending(u32 irq) {
  if (irq < 32) {
    pending_ &= ~(1u << irq);
  }
}

bool InterruptController::is_pending(u32 irq) const {
  if (irq < 32) {
    return (pending_ & (1u << irq)) != 0;
  }
  return false;
}

void InterruptController::set_mask(u32 irq, bool enabled) {
  if (irq < 32) {
    if (enabled) {
      mask_ |= (1u << irq);
    } else {
      mask_ &= ~(1u << irq);
    }
  }
}

bool InterruptController::is_masked(u32 irq) const {
  if (irq < 32) {
    return (mask_ & (1u << irq)) != 0;
  }
  return true;
}

u32 InterruptController::pending_masked() const { return pending_ & mask_; }

int InterruptController::highest_pending() const {
  u32 masked = pending_masked();
  if (masked == 0)
    return -1;

  // Find highest priority (lowest number = highest priority)
  for (int i = 0; i < 32; ++i) {
    if (masked & (1u << i)) {
      return i;
    }
  }
  return -1;
}

void InterruptController::register_handler(u32 irq, Handler handler) {
  if (irq < 32) {
    handlers_[irq] = handler;
  }
}

void InterruptController::unregister_handler(u32 irq) {
  if (irq < 32) {
    handlers_[irq] = nullptr;
  }
}

void InterruptController::handle_interrupts(CPU::State &cpu_state) {
  int irq = highest_pending();
  if (irq >= 0 && handlers_[irq]) {
    handlers_[irq](cpu_state, irq);
    clear_pending(irq);
  }
}

void InterruptController::set_level(u32 level) { level_ = level; }

u32 InterruptController::level() const { return level_; }

} // namespace o2emu::system