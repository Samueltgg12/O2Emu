/**
 * @file mre.cpp
 * @brief MRE (Memory & Rendering Engine) implementation
 */

#include <o2emu/logging/logger.h>
#include <o2emu/memory/mre.h>

namespace o2emu::memory {

MRE::MRE(Memory &memory) : memory_(memory) { reset(); }

void MRE::reset() {
  regs_.fill(0);

  // MRE register defaults (from Linux crmfb driver and IRIX source)
  // MRE_ID: 0x0000
  regs_[REG_ID] = 0x00000001; // MRE revision

  // MRE_CONFIG: 0x0008
  regs_[REG_CONFIG] = 0x00000000;

  // MRE_STATUS: 0x0010
  regs_[REG_STATUS] = 0x00000000;

  // MRE_CONTROL: 0x0018
  regs_[REG_CONTROL] = 0x00000000;

  // Framebuffer configuration
  // MRE_FB_BASE: 0x0100
  regs_[REG_FB_BASE] = 0x00000000;

  // MRE_FB_STRIDE: 0x0108
  regs_[REG_FB_STRIDE] = 0x00001400; // 1280 * 4 bytes = 5120 = 0x1400

  // MRE_FB_WIDTH: 0x0110
  regs_[REG_FB_WIDTH] = 1280;

  // MRE_FB_HEIGHT: 0x0118
  regs_[REG_FB_HEIGHT] = 1024;

  // MRE_FB_DEPTH: 0x0120
  regs_[REG_FB_DEPTH] = 32;

  // MRE_FB_FORMAT: 0x0128
  regs_[REG_FB_FORMAT] = 0x00000001; // RGBA8888

  // Tile configuration (GBE - Graphics Back End)
  // MRE_TILE_CONFIG: 0x0200
  regs_[REG_TILE_CONFIG] = 0x00000000;

  // MRE_TILE_BASE: 0x0208
  regs_[REG_TILE_BASE] = 0x00000000;

  // MRE_TILE_SIZE: 0x0210
  regs_[REG_TILE_SIZE] = 0x00000020; // 32x32 tiles

  // Display list / vertex processing
  // MRE_DL_BASE: 0x0300
  regs_[REG_DL_BASE] = 0x00000000;

  // MRE_DL_PTR: 0x0308
  regs_[REG_DL_PTR] = 0x00000000;

  // MRE_DL_END: 0x0310
  regs_[REG_DL_END] = 0x00000000;

  // MRE_DL_CTRL: 0x0318
  regs_[REG_DL_CTRL] = 0x00000000;

  // Vertex processing
  // MRE_VTX_BASE: 0x0400
  regs_[REG_VTX_BASE] = 0x00000000;

  // MRE_VTX_STRIDE: 0x0408
  regs_[REG_VTX_STRIDE] = 0x00000000;

  // MRE_VTX_COUNT: 0x0410
  regs_[REG_VTX_COUNT] = 0x00000000;

  // MRE_VTX_FORMAT: 0x0418
  regs_[REG_VTX_FORMAT] = 0x00000000;

  // Texture configuration
  // MRE_TEX_BASE: 0x0500
  regs_[REG_TEX_BASE] = 0x00000000;

  // MRE_TEX_STRIDE: 0x0508
  regs_[REG_TEX_STRIDE] = 0x00000000;

  // MRE_TEX_SIZE: 0x0510
  regs_[REG_TEX_SIZE] = 0x00000000;

  // MRE_TEX_FORMAT: 0x0518
  regs_[REG_TEX_FORMAT] = 0x00000000;

  // Rasterization
  // MRE_RASTER_CTRL: 0x0600
  regs_[REG_RASTER_CTRL] = 0x00000000;

  // MRE_SCISSOR: 0x0608
  regs_[REG_SCISSOR] = 0x00000000;

  // MRE_ZBUF_BASE: 0x0610
  regs_[REG_ZBUF_BASE] = 0x00000000;

  // MRE_ZBUF_STRIDE: 0x0618
  regs_[REG_ZBUF_STRIDE] = 0x00000000;

  // Interrupt registers
  // MRE_INT_STATUS: 0x0700
  regs_[REG_INT_STATUS] = 0x00000000;

  // MRE_INT_MASK: 0x0708
  regs_[REG_INT_MASK] = 0x00000000;

  // MRE_INT_CLEAR: 0x0710
  regs_[REG_INT_CLEAR] = 0x00000000;

  // Performance counters
  // MRE_PERF_CTRL: 0x0800
  regs_[REG_PERF_CTRL] = 0x00000000;

  // MRE_PERF_COUNT0-3: 0x0808-0x0820
  for (int i = 0; i < 4; ++i) {
    regs_[REG_PERF_COUNT0 + i] = 0x00000000;
  }
}

u32 MRE::read(u32 offset) const {
  if (offset < sizeof(regs_)) {
    return regs_[offset / 4];
  }
  return 0;
}

void MRE::write(u32 offset, u32 value) {
  if (offset >= sizeof(regs_))
    return;

  u32 reg = offset / 4;

  switch (reg) {
  case REG_CONTROL:
    regs_[reg] = value;
    handle_control_write(value);
    break;

  case REG_FB_BASE:
  case REG_FB_STRIDE:
  case REG_FB_WIDTH:
  case REG_FB_HEIGHT:
  case REG_FB_DEPTH:
  case REG_FB_FORMAT:
    regs_[reg] = value;
    update_framebuffer_config();
    break;

  case REG_TILE_CONFIG:
  case REG_TILE_BASE:
  case REG_TILE_SIZE:
    regs_[reg] = value;
    break;

  case REG_DL_BASE:
  case REG_DL_PTR:
  case REG_DL_END:
  case REG_DL_CTRL:
    regs_[reg] = value;
    if (reg == REG_DL_CTRL && (value & 0x1)) {
      process_display_list();
    }
    break;

  case REG_VTX_BASE:
  case REG_VTX_STRIDE:
  case REG_VTX_COUNT:
  case REG_VTX_FORMAT:
    regs_[reg] = value;
    break;

  case REG_TEX_BASE:
  case REG_TEX_STRIDE:
  case REG_TEX_SIZE:
  case REG_TEX_FORMAT:
    regs_[reg] = value;
    break;

  case REG_RASTER_CTRL:
  case REG_SCISSOR:
  case REG_ZBUF_BASE:
  case REG_ZBUF_STRIDE:
    regs_[reg] = value;
    break;

  case REG_INT_MASK:
    regs_[reg] = value;
    break;

  case REG_INT_CLEAR:
    regs_[REG_INT_STATUS] &= ~value;
    break;

  case REG_PERF_CTRL:
  case REG_PERF_COUNT0:
  case REG_PERF_COUNT1:
  case REG_PERF_COUNT2:
  case REG_PERF_COUNT3:
    regs_[reg] = value;
    break;

  default:
    regs_[reg] = value;
    break;
  }
}

void MRE::handle_control_write([[maybe_unused]] u32 value) {
  // Bit 0: MRE enable
  // Bit 1: Display list enable
  // Bit 2: Tile rendering enable
  // Bit 3: Z-buffer enable
  // Bit 4: Alpha blending enable
  // Bit 5: Texture enable
  // Bit 6: Fog enable
  // Bit 7: Antialiasing enable
  O2EMU_LOG_DEBUG_F("MRE_CONTROL write: 0x{:08x}", value);
}

void MRE::update_framebuffer_config() {
  fb_base_ = regs_[REG_FB_BASE];
  fb_stride_ = regs_[REG_FB_STRIDE];
  fb_width_ = regs_[REG_FB_WIDTH];
  fb_height_ = regs_[REG_FB_HEIGHT];
  fb_depth_ = regs_[REG_FB_DEPTH];
  fb_format_ = regs_[REG_FB_FORMAT];

  O2EMU_LOG_DEBUG_F("Framebuffer config: base=0x{:08x} stride={} {}x{}x{}",
                    fb_base_, fb_stride_, fb_width_, fb_height_, fb_depth_);
}

void MRE::process_display_list() {
  u32 dl_base = regs_[REG_DL_BASE];
  u32 dl_end = regs_[REG_DL_END];
  u32 dl_ptr = regs_[REG_DL_PTR];

  O2EMU_LOG_DEBUG_F(
      "Processing display list: base=0x{:08x} ptr=0x{:08x} end=0x{:08x}",
      dl_base, dl_ptr, dl_end);

  // Simple display list processing - just mark as done for now
  regs_[REG_DL_PTR] = dl_end;
  regs_[REG_STATUS] |= 0x1; // DL done

  // Generate interrupt
  regs_[REG_INT_STATUS] |= 0x1; // DL complete interrupt
}

void MRE::tick(u64 cycles) {
  // Update performance counters
  if (regs_[REG_PERF_CTRL] & 0x1) {
    regs_[REG_PERF_COUNT0] += static_cast<u32>(cycles);
  }
}

u32 MRE::interrupt_status() const {
  return regs_[REG_INT_STATUS] & regs_[REG_INT_MASK];
}

void MRE::clear_interrupt(u32 bit) { regs_[REG_INT_STATUS] &= ~(1 << bit); }

u32 MRE::fb_base() const { return fb_base_; }
u32 MRE::fb_stride() const { return fb_stride_; }
u32 MRE::fb_width() const { return fb_width_; }
u32 MRE::fb_height() const { return fb_height_; }
u32 MRE::fb_depth() const { return fb_depth_; }
u32 MRE::fb_format() const { return fb_format_; }

} // namespace o2emu::memory