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

  // CRIME Render Engine (RE) register offsets, relative to the RE base
  // 0x15000000 (CRM_RE_BASE_ADDRESS). The RE is organized in 4KB pages as
  // documented in docs/register-maps.md (IRIX crimereg.h/crimedef.h).
  enum Register : uint32_t {
    // ---- Page 0: Interface Buffer (CRM_INTFBUF_BASE = 0x0000) ----
    INTFBUF_DATA = 0x0000,  // data[64], 2 words each (0x000..0x1ff)
    INTFBUF_ADDR = 0x0200,  // addr[64] (0x200..0x3ff)
    INTFBUF_CTL = 0x0400,   // control (full/empty/stall levels)
    INTFBUF_RESET = 0x0408, // reset

    // ---- Page 1: TLB (CRM_TLB_BASE = 0x1000) ----
    TLB_FB_A = 0x1000,     // 64 entries (0x1000..0x11ff)
    TLB_FB_B = 0x1200,     // 64 entries (0x1200..0x13ff)
    TLB_FB_C = 0x1400,     // 64 entries (0x1400..0x15ff)
    TLB_TEXTURE = 0x1600,  // 28 entries (0x1600..0x16df)
    TLB_CID = 0x16e0,      // 4 entries (0x16e0..0x16ff)
    TLB_LINEAR_A = 0x1700, // 16 entries (0x1700..0x177f)
    TLB_LINEAR_B = 0x1780, // 16 entries (0x1780..0x17ff)

    // ---- Page 2: Pixel Pipe / Draw (CRM_PIXPIPE_BASE = 0x2000) ----
    PIXPIPE_BUFMODE_SRC = 0x2000,   // source buffer mode
    PIXPIPE_BUFMODE_DST = 0x2008,   // destination buffer mode
    PIXPIPE_CLIPMODE = 0x2010,      // clip mode
    PIXPIPE_DRAWMODE = 0x2018,      // draw mode
    PIXPIPE_SCRMASK = 0x2020,       // screen masks [5] (0x2020..0x2040)
    PIXPIPE_SCISSOR = 0x2048,       // scissor rectangle
    PIXPIPE_WINOFFSET_SRC = 0x2050, // window offset source
    PIXPIPE_WINOFFSET_DST = 0x2058, // window offset dest
    PIXPIPE_PRIMITIVE = 0x2060,     // primitive type/width
    PIXPIPE_VERTEX_X = 0x2070,      // vertex X[3] (0x2070..0x2078)
    PIXPIPE_VERTEX_GL = 0x2080,     // vertex GL[3] (0x2080..0x2094)
    PIXPIPE_STARTSETUP = 0x2098,    // start setup
    PIXPIPE_PIXELXFER_SRC = 0x20a0, // pixel transfer source
    PIXPIPE_PIXELXFER_DST = 0x20b0, // pixel transfer dest
    PIXPIPE_STIPPLE = 0x20c0,       // stipple mode/pattern
    PIXPIPE_SHADE = 0x20d0,         // shade registers (12)
    PIXPIPE_TEXTURE = 0x2110,       // texture registers (23)
    PIXPIPE_FOG = 0x2170,           // fog registers
    PIXPIPE_ANTIALIAS = 0x2190,     // antialias line/coverage
    PIXPIPE_ALPHATEST = 0x2198,     // alpha test
    PIXPIPE_BLEND = 0x21a0,         // blend constant/function
    PIXPIPE_LOGICOP = 0x21b0,       // logic operation
    PIXPIPE_COLORMASK = 0x21b8,     // color mask
    PIXPIPE_DEPTH = 0x21c0,         // depth func/z0/dzdx/dzdy
    PIXPIPE_STENCIL = 0x21e0,       // stencil mode/mask
    PIXPIPE_NULL = 0x21f0,          // null register
    PIXPIPE_FLUSH = 0x21f8,         // flush pipeline

    // ---- Page 3: MTE (CRM_MTE_BASE = 0x3000) ----
    MTE_MODE = 0x3000,        // mode (clear/copy, stipple, depth)
    MTE_BYTEMASK = 0x3008,    // byte mask
    MTE_STIPPLEMASK = 0x3010, // stipple mask
    MTE_FGVALUE = 0x3018,     // foreground value
    MTE_SRC0 = 0x3020,        // source 0
    MTE_SRC1 = 0x3028,        // source 1
    MTE_DST0 = 0x3030,        // destination 0
    MTE_DST1 = 0x3038,        // destination 1
    MTE_SRCYSTEP = 0x3040,    // source Y step
    MTE_DSTYSTEP = 0x3048,    // destination Y step
    MTE_NULL = 0x3070,        // null
    MTE_FLUSH = 0x3078,       // flush

    // ---- Page 4: Status (CRM_STATUS_BASE = 0x4000) ----
    STATUS = 0x4000,        // status register
    SET_START_PTR = 0x4008, // set start pointer
  };

  // Read/write registers (by byte offset)
  u32 read(u32 offset) const;
  void write(u32 offset, u32 value);

  // Render interface
  void start_render();
  void flush_render();
  bool render_busy() const;

  // MTE (Memory Transfer Engine)
  void start_dma(u32 src, u32 dst, u32 size);
  bool dma_busy() const;

  // Reset
  void reset();

  // Tick for performance counters
  void tick(u64 cycles);

  // Interrupt handling
  u32 interrupt_status() const;
  void clear_interrupt(u32 bit);

  // Framebuffer accessors. NOTE: the O2 framebuffer plane is configured by
  // the GBE (0x16030000), not the RE. These accessors are retained for the
  // framebuffer widget and reflect the last values programmed via
  // set_framebuffer(); they are not RE registers.
  u32 fb_base() const;
  u32 fb_stride() const;
  u32 fb_width() const;
  u32 fb_height() const;
  u32 fb_depth() const;
  u32 fb_format() const;
  bool framebuffer_configured() const { return framebuffer_configured_; }

  // Program the framebuffer parameters (used by the GBE plane / widget).
  void set_framebuffer(u32 phys_addr, u32 stride, u32 width, u32 height,
                       u32 depth);

private:
  Memory &memory_;
  std::array<u32, 0x10000 / 4> regs_{}; // 64KB register space

  // Framebuffer state (cached for quick access; mirrors the GBE plane)
  u32 fb_base_ = 0;
  u32 fb_stride_ = 0;
  u32 fb_width_ = 0;
  u32 fb_height_ = 0;
  u32 fb_depth_ = 0;
  u32 fb_format_ = 0;
  bool framebuffer_configured_ = false;

  // State
  bool render_active_ = false;
  bool dma_active_ = false;
};

} // namespace o2emu::memory