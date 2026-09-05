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

  // CRIME register defaults (from Linux driver and IRIX source)
  // CRIME_ID: 0x0000
  regs_[REG_ID] = 0x00000001; // CRIME revision 1

  // CRIME_CONFIG: 0x0008
  regs_[REG_CONFIG] = 0x00000000;

  // CRIME_STATUS: 0x0010
  regs_[REG_STATUS] = 0x00000000;

  // CRIME_CONTROL: 0x0018
  regs_[REG_CONTROL] = 0x00000000;

  // Memory configuration registers
  // CRIME_MEM_CONFIG0-3: 0x0100-0x010C
  for (int i = 0; i < 4; ++i) {
    regs_[REG_MEM_CONFIG0 + i] = 0x00000000;
  }

  // CRIME_REFRESH: 0x0120
  regs_[REG_REFRESH] = 0x00000400; // Default refresh rate

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

  u32 reg = offset / 4;

  switch (reg) {
  case REG_CONTROL:
    // Handle control register writes
    regs_[reg] = value;
    handle_control_write(value);
    break;

  case REG_REFRESH:
    regs_[reg] = value;
    break;

  case REG_ECC_CTRL:
    regs_[reg] = value;
    break;

  case REG_INT_MASK:
    regs_[reg] = value;
    break;

  case REG_INT_CLEAR:
    // Writing 1 clears the corresponding bit in INT_STATUS
    regs_[REG_INT_STATUS] &= ~value;
    break;

  case REG_MEM_CONFIG0:
  case REG_MEM_CONFIG1:
  case REG_MEM_CONFIG2:
  case REG_MEM_CONFIG3:
    regs_[reg] = value;
    break;

  default:
    // Check if it's a DMA register
    if (reg >= REG_DMA_BASE && reg < REG_DMA_BASE + 8 * 6) {
      regs_[reg] = value;
      handle_dma_write(reg - REG_DMA_BASE, value);
    }
    // Check if it's a timer register
    else if (reg >= REG_TIMER_BASE && reg < REG_TIMER_BASE + 2 * 3) {
      regs_[reg] = value;
      handle_timer_write(reg - REG_TIMER_BASE, value);
    } else {
      regs_[reg] = value;
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
  u32 base = REG_DMA_BASE + channel * 6;
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
    regs_[REG_INT_STATUS] |=
        (1 << (16 + channel)); // DMA interrupts at bits 16-23
  }
}

void CRIME::tick(u64 cycles) {
  // Update timers
  for (int t = 0; t < 2; ++t) {
    u32 base = REG_TIMER_BASE + t * 3;
    u32 ctrl = regs_[base + 2];

    if (ctrl & 0x1) {                              // Enabled
      regs_[base + 0] += static_cast<u32>(cycles); // Count

      if (regs_[base + 0] >= regs_[base + 1]) { // Compare
        // Timer expired
        if (ctrl & 0x4) { // Interrupt enable
          regs_[REG_INT_STATUS] |=
              (1 << (8 + t)); // Timer interrupts at bits 8-9
        }

        if (ctrl & 0x2) {      // Periodic
          regs_[base + 0] = 0; // Reset count
        } else {
          regs_[base + 2] &= ~0x1; // Disable timer
        }
      }
    }
  }

  // Memory refresh
  if (regs_[REG_CONTROL] & 0x8) { // Refresh enabled
    refresh_counter_ += cycles;
    u32 refresh_rate = regs_[REG_REFRESH] & 0xFFFF;
    if (refresh_counter_ >= refresh_rate * 1000) { // Approximate
      refresh_counter_ = 0;
      // Perform refresh (handled by memory controller)
    }
  }
}

u32 CRIME::interrupt_status() const {
  return regs_[REG_INT_STATUS] & regs_[REG_INT_MASK];
}

void CRIME::clear_interrupt(u32 bit) { regs_[REG_INT_STATUS] &= ~(1 << bit); }

} // namespace o2emu::memory