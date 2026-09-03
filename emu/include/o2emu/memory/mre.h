#pragma once

/**
 * @file mre.h
 * @brief MRE (Memory & Rendering Engine) - part of CRM chipset
 *
 * Based on PROM decompiled definitions.h (BASE_RENDER = 0x15000000)
 * and Linux/NetBSD driver sources
 */

#include <array>
#include <o2emu/o2emu.h>

namespace o2emu::memory {

class MRE {
public:
  MRE();
  ~MRE() = default;

  // MRE register offsets (from PHYS_BASE_RENDER = 0x15000000)
  // Based on PROM definitions.h and register-maps.md
  enum Register : uint32_t {
    // Render Interface
    RENDER_INTF_BASE = 0x000000,
    RENDER_INTF_STATUS = 0x000000,
    RENDER_INTF_CONTROL = 0x000004,
    RENDER_INTF_START = 0x000008,
    RENDER_INTF_FLUSH = 0x00000C,

    // Render TLB
    RENDER_TLB_BASE = 0x001000,
    RENDER_TLB_ENTRY = 0x001000, // 64 entries

    // Display Engine (DE)
    DE_BASE = 0x002000,
    DE_CONTROL = 0x002000,
    DE_STATUS = 0x002004,
    DE_FB_BASE = 0x002008,
    DE_FB_STRIDE = 0x00200C,
    DE_TILE_CONFIG = 0x002010,

    // Memory Transfer Engine (MTE)
    MTE_BASE = 0x003000,
    MTE_CONTROL = 0x003000,
    MTE_SRC_ADDR = 0x003004,
    MTE_DST_ADDR = 0x003008,
    MTE_SIZE = 0x00300C,
    MTE_STATUS = 0x003010,

    // Status/Control
    RENDER_STATUS = 0x00FF00,
    RENDER_RESET = 0x00FF04,
    RENDER_REVISION = 0x00FFFC,
  };

  // Read/write registers
  u32 read(Register reg);
  void write(Register reg, u32 value);

  // Render interface
  void start_render();
  void flush_render();
  bool render_busy() const;

  // Display Engine
  void set_framebuffer(u32 phys_addr, u32 stride, u32 width, u32 height,
                       u32 depth);
  void set_tile_config(u32 config);

  // MTE (Memory Transfer Engine)
  void start_dma(u32 src, u32 dst, u32 size);
  bool dma_busy() const;

  // Reset
  void reset();

private:
  std::array<u32, 0x10000 / 4> regs_ = {}; // 64KB register space

  // State
  bool render_active_ = false;
  bool dma_active_ = false;
};

} // namespace o2emu::memory