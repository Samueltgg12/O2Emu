/**
 * @file ice.h
 * @brief ICE (Imaging & Compression Engine) = VICE ASIC
 *
 * The ICE is the VICE (Video Image Compression Engine). Register map sourced
 * from the VICE Design Specification 099-0123-003 v1.0
 * (docs/manuals-specs/o2-VICE-spec.md), transcribed in docs/register-maps.md
 * under "VICE / ICE". With VICE_ID pins = 00 the chip decodes SysAD bits
 * (21:20) and occupies 0x17000000-0x170FFFFC.
 */

#pragma once

#include <array>
#include <o2emu/devices/device.h>
#include <o2emu/o2emu.h>

namespace o2emu::graphics {

class ICE : public devices::Device {
public:
  ICE();
  ~ICE() override = default;

  // VICE system base address (VICE_ID pins = 00).
  static constexpr u32 VICE_BASE = 0x17000000;

  // VICE register offsets, relative to 0x17000000. All registers sit on
  // double-word (8-byte) boundaries regardless of width.
  enum Register : uint32_t {
    // Chip registers (0x0008..0x01F8)
    VICE_ID = 0x0008,           // chip rev/ID (r, reset 0xE1, 8 bits)
    VICE_CFG = 0x0020,          // general config (r/w, 16 bits)
    HST_BSP_IN_BOX = 0x0028,    // host copy of BSP/MSP in mailbox (r, 16)
    HST_BSP_OUT_BOX = 0x0030,   // host copy of BSP/MSP out mailbox (r, 16)
    MSP_CTL_STAT = 0x0040,      // MSP control/status (r/w, 32)
    MSP_ExcpFlag = 0x0048,      // MSP exception flag (r/w, 32)
    MSP_PC = 0x0050,            // MSP program counter (r/w, 32)
    MSP_BadAddr = 0x0058,       // MSP bad address (r, 32)
    MSP_WatchPoint = 0x0060,    // MSP watchpoint (r/w, 32)
    MSP_EPC = 0x0068,           // MSP exception PC (r, 32)
    MSP_CAUSE = 0x0070,         // MSP exception cause (r, 32)
    BSP_RPAGE = 0x0078,         // BSP R page (r/w, 16)
    BSP_SW_INT = 0x0080,        // BSP software interrupt (w)
    MSP_D_RAM = 0x0100,         // MSP data RAM arbitration (r/w, 32)
    VICEMSP_COUNT = 0x0108,     // MSP free-running counter (r, 32)
    BSP_CTL_STAT = 0x0110,      // BSP control/status (r/w, 16)
    BSP_WatchPoint = 0x0118,    // BSP watchpoint (r/w, 16)
    BSP_IN_COUNT = 0x0120,      // BSP decoded bits counter (r, 24)
    BSP_OUT_COUNT = 0x0128,     // BSP encoded bits counter (r, 24)
    BSP_PC = 0x0140,            // BSP program counter (r/w, 16)
    BSP_EPC = 0x0148,           // BSP exception PC (r, 16)
    BSP_HALT_RESET = 0x0150,    // BSP halt/reset control (r, 2)
    BSP_CAUSE = 0x0158,         // BSP exception cause (r, 16)
    VICE_INT = 0x0160,          // interrupt status (r, 9)
    BSP_FIFO_CTL_STAT = 0x0168, // BSP FIFO control/status (r/w, 6)
    BSP_AVALID_BITS = 0x0170,   // BSP A FIFO valid bits (r/w)
    BSP_FVALID_BITS = 0x0178,   // BSP F FIFO valid bits (r/w)
    DMA_CTL_CH1 = 0x0180,       // DMA ch1 control (r/w, 16)
    DMA_STAT_CH1 = 0x0188,      // DMA ch1 status (r, 16)
    DMA_DATA_CH1 = 0x0190,      // DMA ch1 data fill (r/w, 16)
    DMA_MEM_PT_CH1 = 0x0198,    // DMA ch1 system pointer (r, 32)
    DMA_VICE_PT_CH1 = 0x01A0,   // DMA ch1 VICE pointer (r, 16)
    DMA_COUNT_CH1 = 0x01A8,     // DMA ch1 remaining count (r, 16)
    MSP_SW_INT = 0x01B8,        // MSP software interrupt (w)
    DMA_CTL_CH2 = 0x01C0,       // DMA ch2 control (r/w, 16)
    DMA_STAT_CH2 = 0x01C8,      // DMA ch2 status (r, 16)
    DMA_DATA_CH2 = 0x01D0,      // DMA ch2 data fill (r/w, 16)
    DMA_MEM_PT_CH2 = 0x01D8,    // DMA ch2 system pointer (r, 32)
    DMA_VICE_PT_CH2 = 0x01E0,   // DMA ch2 VICE pointer (r, 16)
    DMA_COUNT_CH2 = 0x01E8,     // DMA ch2 remaining count (r, 16)
    BSP_IN_BOX = 0x01F0,        // BSP/MSP in mailbox (r, 16)
    BSP_OUT_BOX = 0x01F8,       // BSP/MSP out mailbox (r/w, 16)

    // Kernel-restricted registers (0xE000..0xE008)
    VICE_CFG_KERN = 0xE000,  // VICE_CFG (kernel alias)
    VICE_INT_RESET = 0xE008, // interrupt reset (w1c, 9)
    VICE_INT_EN = 0xE010,    // interrupt enable (r/w, 9)

    // DMA descriptor sets (0x1000..0x11FF)
    DMA_CH1_D1 = 0x1000, // ch1 descriptor set 1
    DMA_CH1_D2 = 0x1040, // ch1 descriptor set 2
    DMA_CH1_D3 = 0x1080, // ch1 descriptor set 3
    DMA_CH1_D4 = 0x10C0, // ch1 descriptor set 4
    DMA_CH2_D1 = 0x1100, // ch2 descriptor set 1
    DMA_CH2_D2 = 0x1140, // ch2 descriptor set 2
    DMA_CH2_D3 = 0x1180, // ch2 descriptor set 3
    DMA_CH2_D4 = 0x11C0, // ch2 descriptor set 4

    // VICE TLB (0xF000..0xFFFF, 64 entries)
    VICE_TLB = 0xF000,
  };

  // Device interface (bus-mapped at 0x17000000)
  u32 read32(u32 offset) override;
  u16 read16(u32 offset) override;
  u8 read8(u32 offset) override;
  void write32(u32 offset, u32 value) override;
  void write16(u32 offset, u16 value) override;
  void write8(u32 offset, u8 value) override;
  void reset() override;

  // Status
  bool busy() const;
  u32 status() const;
  u32 interrupt_status() const;

private:
  std::array<u32, 0x10000 / 4> regs_{}; // 64KB register space

  // State
  bool busy_ = false;
  u32 interrupt_enable_ = 0;
  u32 interrupt_status_ = 0;
};

} // namespace o2emu::graphics