/**
 * @file mace_isa.cpp
 * @brief MACE ISA bridge implementation
 */

#include <o2emu/devices/mace/mace_isa.h>
#include <o2emu/logging/logger.h>

namespace o2emu::devices {

MACEISA::MACEISA(MACE &mace)
    : Device("MACEISA", 0x20000, 0x10000), mace_(mace) {
  reset();
}

MACEISA::~MACEISA() = default;

void MACEISA::reset() {
  regs_.fill(0);

  // MACE ISA register defaults
  // ISA_CTRL: 0x0000
  regs_[REG_CTRL] = 0x00000000;

  // ISA_STATUS: 0x0008
  regs_[REG_STATUS] = 0x00000000;

  // ISA_IO_BASE: 0x0010
  regs_[REG_IO_BASE] = 0x00000000;

  // ISA_MEM_BASE: 0x0018
  regs_[REG_MEM_BASE] = 0x00000000;

  // ISA_DMA_CTRL: 0x0100
  regs_[REG_DMA_CTRL] = 0x00000000;

  // ISA_DMA_ADDR: 0x0108
  regs_[REG_DMA_ADDR] = 0x00000000;

  // ISA_DMA_COUNT: 0x0110
  regs_[REG_DMA_COUNT] = 0x00000000;

  // ISA_DMA_STATUS: 0x0118
  regs_[REG_DMA_STATUS] = 0x00000000;

  // ISA_TIMER_CTRL: 0x0200
  regs_[REG_TIMER_CTRL] = 0x00000000;

  // ISA_TIMER_COUNT: 0x0208
  regs_[REG_TIMER_COUNT] = 0x00000000;

  // ISA_INT_STATUS: 0x0300
  regs_[REG_INT_STATUS] = 0x00000000;

  // ISA_INT_MASK: 0x0308
  regs_[REG_INT_MASK] = 0x00000000;

  // ISA_INT_CLEAR: 0x0310
  regs_[REG_INT_CLEAR] = 0x00000000;

  // PS/2 keyboard/mouse
  // ISA_PS2_KBD_DATA: 0x0400
  regs_[REG_PS2_KBD_DATA] = 0x00000000;

  // ISA_PS2_KBD_CTRL: 0x0408
  regs_[REG_PS2_KBD_CTRL] = 0x00000000;

  // ISA_PS2_MOUSE_DATA: 0x0410
  regs_[REG_PS2_MOUSE_DATA] = 0x00000000;

  // ISA_PS2_MOUSE_CTRL: 0x0418
  regs_[REG_PS2_MOUSE_CTRL] = 0x00000000;

  // Serial ports (2x 16550 compatible)
  for (int i = 0; i < 2; ++i) {
    u32 base = REG_UART_BASE + i * 0x100;
    for (int j = 0; j < 8; ++j) {
      regs_[base + j] = 0;
    }
    // UART LCR default
    regs_[base + 3] = 0x03; // 8N1
  }

  // Parallel port
  // ISA_PAR_DATA: 0x0600
  regs_[REG_PAR_DATA] = 0x00000000;

  // ISA_PAR_STATUS: 0x0608
  regs_[REG_PAR_STATUS] = 0x00000000;

  // ISA_PAR_CTRL: 0x0610
  regs_[REG_PAR_CTRL] = 0x00000000;
}

u32 MACEISA::read_reg(u32 offset) {
  if (offset < sizeof(regs_)) {
    return regs_[offset / 4];
  }
  return 0;
}

void MACEISA::write_reg(u32 offset, u32 value) {
  if (offset >= sizeof(regs_))
    return;

  u32 reg = offset / 4;

  switch (reg) {
  case REG_CTRL:
    regs_[reg] = value;
    break;

  case REG_IO_BASE:
  case REG_MEM_BASE:
    regs_[reg] = value;
    break;

  case REG_DMA_CTRL:
  case REG_DMA_ADDR:
  case REG_DMA_COUNT:
    regs_[reg] = value;
    if (reg == REG_DMA_CTRL && (value & 0x1)) {
      start_dma();
    }
    break;

  case REG_TIMER_CTRL:
  case REG_TIMER_COUNT:
    regs_[reg] = value;
    break;

  case REG_PS2_KBD_DATA:
  case REG_PS2_KBD_CTRL:
  case REG_PS2_MOUSE_DATA:
  case REG_PS2_MOUSE_CTRL:
    regs_[reg] = value;
    break;

  case REG_UART_BASE ... REG_UART_BASE + 0x1FF:
    // UART registers
    regs_[reg] = value;
    break;

  case REG_PAR_DATA:
  case REG_PAR_STATUS:
  case REG_PAR_CTRL:
    regs_[reg] = value;
    break;

  case REG_INT_MASK:
    regs_[reg] = value;
    break;

  case REG_INT_CLEAR:
    regs_[REG_INT_STATUS] &= ~value;
    break;

  default:
    regs_[reg] = value;
    break;
  }
}

void MACEISA::config_write(u32 reg, u32 value) {
  // Handle ISA config writes from MACE
  switch (reg) {
  case 0x2000: // ISA_CONFIG
    break;
  case 0x2008: // ISA_IO_BASE
    regs_[REG_IO_BASE] = value;
    break;
  case 0x2010: // ISA_MEM_BASE
    regs_[REG_MEM_BASE] = value;
    break;
  }
}

void MACEISA::start_dma() {
  u32 dma_addr = regs_[REG_DMA_ADDR];
  u32 dma_count = regs_[REG_DMA_COUNT];
  u32 dma_ctrl = regs_[REG_DMA_CTRL];

  O2EMU_LOG_DEBUG("ISA DMA start: addr=0x"
                  << std::hex << dma_addr << " count=" << dma_count
                  << " ctrl=0x" << dma_ctrl << std::dec);

  // Simulate DMA completion
  regs_[REG_DMA_STATUS] = 0x1;
  regs_[REG_INT_STATUS] |= 0x1; // DMA complete
}

bool MACEISA::read(u32 offset, [[maybe_unused]] u32 size, u32 &value) {
  if (offset < sizeof(regs_)) {
    value = read_reg(offset);
    return true;
  }
  return false;
}

bool MACEISA::write(u32 offset, [[maybe_unused]] u32 size, u32 value) {
  if (offset < sizeof(regs_)) {
    write_reg(offset, value);
    return true;
  }
  return false;
}

void MACEISA::tick(u64 cycles) {
  // Update ISA timer
  if (regs_[REG_TIMER_CTRL] & 0x1) {
    regs_[REG_TIMER_COUNT] += static_cast<u32>(cycles);
    if (regs_[REG_TIMER_COUNT] >= 0x10000) { // Approximate 1ms at 133MHz
      regs_[REG_TIMER_COUNT] = 0;
      regs_[REG_INT_STATUS] |= 0x2; // Timer interrupt
    }
  }
}

u32 MACEISA::interrupt_status() const {
  return regs_[REG_INT_STATUS] & regs_[REG_INT_MASK];
}

void MACEISA::clear_interrupt(u32 mask) { regs_[REG_INT_STATUS] &= ~mask; }

} // namespace o2emu::devices