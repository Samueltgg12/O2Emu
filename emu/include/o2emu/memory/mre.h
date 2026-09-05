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

class Memory;

class MRE {
public:
  explicit MRE(Memory &memory);
  ~MRE();

  // MRE register offsets (from PHYS_BASE_RENDER = 0x15000000)
  // Based on PROM definitions.h and register-maps.md
  enum Register : uint32_t {
    // Core registers
    REG_ID = 0x0000,
    REG_CONFIG = 0x0008,
    REG_STATUS = 0x0010,
    REG_CONTROL = 0x0018,

    // Framebuffer configuration
    REG_FB_BASE = 0x0100,
    REG_FB_STRIDE = 0x0108,
    REG_FB_WIDTH = 0x0110,
    REG_FB_HEIGHT = 0x0118,
    REG_FB_DEPTH = 0x0120,
    REG_FB_FORMAT = 0x0128,

    // Tile configuration (GBE - Graphics Back End)
    REG_TILE_CONFIG = 0x0200,
    REG_TILE_BASE = 0x0208,
    REG_TILE_SIZE = 0x0210,

    // Display list / vertex processing
    REG_DL_BASE = 0x0300,
    REG_DL_PTR = 0x0308,
    REG_DL_END = 0x0310,
    REG_DL_CTRL = 0x0318,

    // Vertex processing
    REG_VTX_BASE = 0x0400,
    REG_VTX_STRIDE = 0x0408,
    REG_VTX_COUNT = 0x0410,
    REG_VTX_FORMAT = 0x0418,

    // Texture configuration
    REG_TEX_BASE = 0x0500,
    REG_TEX_STRIDE = 0x0508,
    REG_TEX_SIZE = 0x0510,
    REG_TEX_FORMAT = 0x0518,

    // Rasterization
    REG_RASTER_CTRL = 0x0600,
    REG_SCISSOR = 0x0608,
    REG_ZBUF_BASE = 0x0610,
    REG_ZBUF_STRIDE = 0x0618,

    // Interrupt registers
    REG_INT_STATUS = 0x0700,
    REG_INT_MASK = 0x0708,
    REG_INT_CLEAR = 0x0710,

    // Performance counters
    REG_PERF_CTRL = 0x0800,
    REG_PERF_COUNT0 = 0x0808,
    REG_PERF_COUNT1 = 0x0810,
    REG_PERF_COUNT2 = 0x0818,
    REG_PERF_COUNT3 = 0x0820,

    // Legacy render interface (for compatibility)
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

  // Read/write registers (by byte offset)
  u32 read(u32 offset) const;
  void write(u32 offset, u32 value);

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

  // Display list processing
  void process_display_list();

  // Tick for performance counters
  void tick(u64 cycles);

  // Interrupt handling
  u32 interrupt_status() const;
  void clear_interrupt(u32 bit);

  // Framebuffer accessors
  u32 fb_base() const;
  u32 fb_stride() const;
  u32 fb_width() const;
  u32 fb_height() const;
  u32 fb_depth() const;
  u32 fb_format() const;

private:
  Memory &memory_;
  std::array<u32, 0x10000 / 4> regs_{}; // 64KB register space

  // Framebuffer state (cached for quick access)
  u32 fb_base_ = 0;
  u32 fb_stride_ = 0;
  u32 fb_width_ = 0;
  u32 fb_height_ = 0;
  u32 fb_depth_ = 0;
  u32 fb_format_ = 0;

  // State
  bool render_active_ = false;
  bool dma_active_ = false;

  // Internal helpers
  void handle_control_write(u32 value);
  void update_framebuffer_config();
};

} // namespace o2emu::memory