/**
 * @file crime.cpp
 * @brief CRIME (Memory Controller) implementation
 */

#include <cstring>
#include <o2emu/logging/logger.h>
#include <o2emu/memory/crime.h>

namespace o2emu::memory {

CRIME::CRIME(Memory &memory) : memory_(memory) { reset(); }

void CRIME::reset() {
  regs_.fill(0);

  // CRIME_ID: IDVALUE 0xa0, revision in bits 3:0 (Linux crime.h).
  regs_[REG_ID / 4] = 0x000000a1;
  regs_[REG_CONTROL / 4] = 0;
  regs_[REG_INTSTAT / 4] = 0;
  regs_[REG_INTMASK / 4] = 0;
  regs_[REG_MEM_REFRESH_CNTR / 4] = 0x00000400;

  // CRIME_ECC_CTRL: 0x0130
  regs_[REG_ECC_CTRL] = 0x00000000;

  // CRIME_ECC_STATUS: 0x0138
  regs_[REG_ECC_STATUS] = 0x00000000;

  // CRIME_ECC_ADDR: 0x0140
  regs_[REG_ECC_ADDR] = 0x00000000;

  // CRIME_ECC_SYNDROME: 0x0148
  regs_[REG_ECC_SYNDROME] = 0x00000000;

  // Interrupt registers
  // CRIME_INT_STATUS: 0x0200
  regs_[REG_INT_STATUS] = 0x00000000;

  // CRIME_INT_MASK: 0x0208
  regs_[REG_INT_MASK] = 0x00000000;

  // CRIME_INT_CLEAR: 0x0210
  regs_[REG_INT_CLEAR] = 0x00000000;

  // DMA registers (8 channels)
  for (int ch = 0; ch < 8; ++ch) {
    u32 base = REG_DMA_BASE + ch * 0x40;
    regs_[base + 0] = 0; // DMA_SRC
    regs_[base + 1] = 0; // DMA_DST
    regs_[base + 2] = 0; // DMA_COUNT
    regs_[base + 3] = 0; // DMA_CTRL
    regs_[base + 4] = 0; // DMA_NEXT
    regs_[base + 5] = 0; // DMA_STATUS
  }

  // Timer registers (2 timers)
  for (int t = 0; t < 2; ++t) {
    u32 base = REG_TIMER_BASE + t * 0x20;
    regs_[base + 0] = 0; // TIMER_COUNT
    regs_[base + 1] = 0; // TIMER_COMPARE
    regs_[base + 2] = 0; // TIMER_CTRL
  }
}

u32 CRIME::read(u32 offset) const {
  if (offset < sizeof(regs_)) {
    return regs_[offset / 4];
  }
  return 0;
}

void CRIME::write(u32 offset, u32 value) {
  if (offset >= sizeof(regs_))
    return;

  switch (offset) {
  case REG_CONTROL:
    regs_[offset / 4] = value;
    handle_control_write(value);
    break;
  case REG_INTMASK:
    regs_[offset / 4] = value;
    intr_mask_ = value;
    break;
  case REG_SOFTINT:
    regs_[REG_INTSTAT / 4] &= ~value;
    break;
  default:
    regs_[offset / 4] = value;
    if (offset >= REG_DMA_BASE && offset < REG_DMA_BASE + 8 * 0x18) {
      handle_dma_write((offset - REG_DMA_BASE) / 4, value);
    } else if (offset >= REG_TIMER_BASE && offset < REG_TIMER_BASE + 2 * 0x0C) {
      handle_timer_write((offset - REG_TIMER_BASE) / 4, value);
    }
    break;
  }
}

void CRIME::handle_control_write(u32 value) {
  // Bit 0: Memory enable
  // Bit 1: ECC enable
  // Bit 2: Scrub enable
  // Bit 3: Refresh enable
  O2EMU_LOG_DEBUG_F("CRIME_CONTROL write: 0x{:x}", value);
}

void CRIME::handle_dma_write(u32 reg_offset, u32 value) {
  u32 channel = reg_offset / 6;
  u32 reg = reg_offset % 6;

  if (reg == 3) { // DMA_CTRL
    // Bit 0: Enable
    // Bit 1: Direction (0=memory->device, 1=device->memory)
    // Bit 2: Interrupt on completion
    // Bit 3: Chain to next descriptor
    if (value & 0x1) {
      start_dma(channel);
    }
  }
}

void CRIME::handle_timer_write(u32 reg_offset, u32 value) {
  u32 timer = reg_offset / 3;
  u32 reg = reg_offset % 3;

  if (reg == 2) { // TIMER_CTRL
    // Bit 0: Enable
    // Bit 1: Periodic mode
    // Bit 2: Interrupt enable
    O2EMU_LOG_DEBUG_F("CRIME Timer {} control: 0x{:x}", timer, value);
  }
}

void CRIME::start_dma(u32 channel) {
  u32 base = (REG_DMA_BASE / 4) + channel * 6;
  u32 src = regs_[base + 0];
  u32 dst = regs_[base + 1];
  u32 count = regs_[base + 2];
  u32 ctrl = regs_[base + 3];

  O2EMU_LOG_DEBUG_F("DMA channel {} start: src=0x{:x} dst=0x{:x} count={}",
                    channel, src, dst, count);

  // Simple DMA implementation - just copy memory
  bool to_device = (ctrl >> 1) & 1;

  if (to_device) {
    // Memory to device - not implemented yet
  } else {
    // Device to memory - not implemented yet
  }

  // Mark as complete
  regs_[base + 5] = 0x1; // Status: done

  // Generate interrupt if enabled
  if (ctrl & 0x4) {
    regs_[REG_INTSTAT / 4] |= (1u << (16 + channel));
  }
}

void CRIME::tick(u64 cycles) {
  // CRM_TIME runs at ~66.67 MHz (half the 133 MHz UMA bus). Count in the
  // same units as CPU cycles so firmware polling makes progress.
  u64 time = (static_cast<u64>(regs_[(REG_TIME / 4)]) << 32) |
             regs_[(REG_TIME / 4) + 1];
  time += cycles;
  regs_[REG_TIME / 4] = static_cast<u32>(time >> 32);
  regs_[(REG_TIME / 4) + 1] = static_cast<u32>(time);

  for (int t = 0; t < 2; ++t) {
    u32 base = (REG_TIMER_BASE / 4) + t * 3;
    u32 ctrl = regs_[base + 2];

    if (ctrl & 0x1) {
      regs_[base + 0] += static_cast<u32>(cycles);

      if (regs_[base + 0] >= regs_[base + 1]) {
        if (ctrl & 0x4) {
          regs_[REG_INTSTAT / 4] |= (1u << (8 + t));
        }

        if (ctrl & 0x2) {
          regs_[base + 0] = 0;
        } else {
          regs_[base + 2] &= ~0x1;
        }
      }
    }
  }

  // Memory refresh
  if (regs_[REG_CONTROL / 4] & 0x8) {
    refresh_counter_ += cycles;
    u32 refresh_rate = regs_[REG_MEM_REFRESH_CNTR / 4] & 0xFFFF;
    if (refresh_counter_ >= refresh_rate * 1000) { // Approximate
      refresh_counter_ = 0;
      // Perform refresh (handled by memory controller)
    }
  }
}

u32 CRIME::interrupt_status() const {
  return regs_[REG_INTSTAT / 4] & regs_[REG_INTMASK / 4];
}

void CRIME::clear_interrupt(u32 bit) {
  regs_[REG_INTSTAT / 4] &= ~(1u << bit);
}

} // namespace o2emu::memory