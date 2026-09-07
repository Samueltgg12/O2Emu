/**
 * @file ice.cpp
 * @brief ICE (Imaging & Compression Engine) = VICE ASIC implementation
 *
 * Register map sourced from the VICE Design Specification 099-0123-003 v1.0
 * (docs/manuals-specs/o2-VICE-spec.md), transcribed in docs/register-maps.md
 * under "VICE / ICE". The chip decodes SysAD bits (21:20) against VICE_ID
 * pins; with VICE_ID = 00 it occupies 0x17000000-0x170FFFFC.
 */

#include <o2emu/graphics/ice.h>
#include <o2emu/logging/logger.h>

namespace o2emu::graphics {

ICE::ICE() : devices::Device("VICE (ICE)", VICE_BASE, 0x10000) { reset(); }

void ICE::reset() {
  regs_.fill(0);
  busy_ = false;
  interrupt_enable_ = 0;
  interrupt_status_ = 0;

  // VICE_ID reads back the chip revision/ID (reset value 0xE1 per spec).
  regs_[VICE_ID / 4] = 0xE1;
  // BSP FIFO control/status resets to 0x05 per spec.
  regs_[BSP_FIFO_CTL_STAT / 4] = 0x05;
  // DMA channel control/status resets to 0x10 per spec.
  regs_[DMA_CTL_CH1 / 4] = 0x10;
  regs_[DMA_STAT_CH1 / 4] = 0x10;
  regs_[DMA_CTL_CH2 / 4] = 0x10;
  regs_[DMA_STAT_CH2 / 4] = 0x10;
}

u32 ICE::read32(u32 offset) {
  if (offset >= regs_.size() * 4)
    return 0;

  switch (offset) {
  case VICE_INT:
    // Interrupt status register (9 bits).
    return interrupt_status_;
  default:
    return regs_[offset / 4];
  }
}

u16 ICE::read16(u32 offset) {
  return static_cast<u16>(read32(offset) >> ((offset & 2u) * 8u));
}

u8 ICE::read8(u32 offset) {
  return static_cast<u8>(read32(offset) >> ((offset & 3u) * 8u));
}

void ICE::write32(u32 offset, u32 value) {
  if (offset >= regs_.size() * 4)
    return;

  switch (offset) {
  case VICE_INT_RESET:
    // Interrupt reset: write-1-to-clear (9 bits).
    interrupt_status_ &= ~value;
    break;

  case VICE_INT_EN:
    // Interrupt enable (9 bits).
    interrupt_enable_ = value;
    break;

  case BSP_SW_INT:
  case MSP_SW_INT:
    // Software interrupt: writing raises the corresponding interrupt.
    interrupt_status_ |= 0x1;
    break;

  default:
    regs_[offset / 4] = value;
    break;
  }
}

void ICE::write16(u32 offset, u16 value) {
  const u32 aligned = offset & ~3u;
  u32 current = read32(aligned);
  const u32 shift = (offset & 2u) * 8u;
  current =
      (current & ~(0xffffu << shift)) | (static_cast<u32>(value) << shift);
  write32(aligned, current);
}

void ICE::write8(u32 offset, u8 value) {
  const u32 aligned = offset & ~3u;
  u32 current = read32(aligned);
  const u32 shift = (offset & 3u) * 8u;
  current = (current & ~(0xffu << shift)) | (static_cast<u32>(value) << shift);
  write32(aligned, current);
}

bool ICE::busy() const { return busy_; }

u32 ICE::status() const {
  // MSP_CTL_STAT reflects the MSP control/status word.
  return regs_[MSP_CTL_STAT / 4];
}

u32 ICE::interrupt_status() const {
  return interrupt_status_ & interrupt_enable_;
}

} // namespace o2emu::graphics