/**
 * @file display_engine.h
 * @brief Display Engine (analog video out) - part of CRM chipset
 *
 * Based on IRIX crm_de.h and PROM definitions
 * The Display Engine handles video timing, cursor, and analog output
 */

#pragma once

#include <array>
#include <o2emu/o2emu.h>

namespace o2emu::graphics {

class DisplayEngine {
public:
  DisplayEngine();
  ~DisplayEngine() = default;

  // Display Engine register offsets (from PHYS_BASE_GBE = 0x16000000)
  // Based on IRIX crm_de.h and PROM definitions
  enum Register : uint32_t {
    // Control/Status
    DE_CONTROL = 0x000000,
    DE_STATUS = 0x000004,
    DE_INTERRUPT = 0x000008,
    DE_INTERRUPT_MASK = 0x00000C,

    // Video timing
    DE_TIMING_BASE = 0x001000,
    DE_H_TOTAL = 0x001000,
    DE_H_DISPLAY = 0x001004,
    DE_H_SYNC_START = 0x001008,
    DE_H_SYNC_END = 0x00100C,
    DE_V_TOTAL = 0x001010,
    DE_V_DISPLAY = 0x001014,
    DE_V_SYNC_START = 0x001018,
    DE_V_SYNC_END = 0x00101C,
    DE_V_SYNC_START_ODD = 0x001020, // For interlaced
    DE_V_SYNC_END_ODD = 0x001024,

    // Framebuffer
    DE_FB_BASE = 0x002000,
    DE_FB_ADDR = 0x002000,
    DE_FB_STRIDE = 0x002004,
    DE_FB_FORMAT = 0x002008,
    DE_FB_WIDTH = 0x00200C,
    DE_FB_HEIGHT = 0x002010,
    DE_FB_OFFSET_X = 0x002014,
    DE_FB_OFFSET_Y = 0x002018,

    // Cursor
    DE_CURSOR_BASE = 0x003000,
    DE_CURSOR_CONTROL = 0x003000,
    DE_CURSOR_POS_X = 0x003004,
    DE_CURSOR_POS_Y = 0x003008,
    DE_CURSOR_HOT_X = 0x00300C,
    DE_CURSOR_HOT_Y = 0x003010,
    DE_CURSOR_COLOR_1 = 0x003014,
    DE_CURSOR_COLOR_2 = 0x003018,
    DE_CURSOR_COLOR_3 = 0x00301C,
    DE_CURSOR_PATTERN = 0x003100, // 64x64 2bpp pattern

    // Video DAC / Gamma
    DE_DAC_BASE = 0x004000,
    DE_DAC_CONTROL = 0x004000,
    DE_GAMMA_RED = 0x004100,   // 256 entries
    DE_GAMMA_GREEN = 0x004200, // 256 entries
    DE_GAMMA_BLUE = 0x004300,  // 256 entries

    // Tile configuration (GBE)
    DE_TILE_BASE = 0x005000,
    DE_TILE_CONFIG = 0x005000,
    DE_TILE_FB_BASE = 0x005004,
    DE_TILE_FB_STRIDE = 0x005008,

    // Status/Control
    DE_RESET = 0x00FF00,
    DE_REVISION = 0x00FFFC,
  };

  // Control bits
  enum ControlBit : uint32_t {
    CTRL_ENABLE = 0x00000001,
    CTRL_RESET = 0x00000002,
    CTRL_INT_ENABLE = 0x00000004,
    CTRL_INTERLACE = 0x00000010,
    CTRL_SYNC_ON_GREEN = 0x00000020,
    CTRL_COMPOSITE_SYNC = 0x00000040,
    CTRL_CURSOR_ENABLE = 0x00000100,
    CTRL_CURSOR_64x64 = 0x00000200,
    CTRL_GAMMA_ENABLE = 0x00001000,
    CTRL_TILE_ENABLE = 0x00010000,
  };

  // Status bits
  enum StatusBit : uint32_t {
    STATUS_VBLANK = 0x00000001,
    STATUS_HBLANK = 0x00000002,
    STATUS_ODD_FIELD = 0x00000004,
    STATUS_VSYNC = 0x00000008,
    STATUS_HSYNC = 0x00000010,
    STATUS_CURSOR_VISIBLE = 0x00000100,
    STATUS_FB_ACTIVE = 0x00001000,
  };

  // Interrupt bits
  enum InterruptBit : uint32_t {
    INTR_VBLANK = 0x00000001,
    INTR_HBLANK = 0x00000002,
    INTR_VSYNC = 0x00000004,
    INTR_LINE_COMPARE = 0x00000008,
    INTR_CURSOR = 0x00000010,
    INTR_ERROR = 0x00000020,
  };

  // Framebuffer formats
  enum Format : uint32_t {
    FMT_8BPP = 0x00,         // 8-bit indexed
    FMT_16BPP = 0x01,        // 16-bit RGB 565
    FMT_24BPP = 0x02,        // 24-bit RGB 888
    FMT_32BPP = 0x03,        // 32-bit RGBA 8888
    FMT_24BPP_PACKED = 0x04, // Packed 24-bit
  };

  // Cursor control bits
  enum CursorControlBit : uint32_t {
    CURSOR_ENABLE = 0x00000001,
    CURSOR_64x64 = 0x00000002,
    CURSOR_128x128 = 0x00000004,
    CURSOR_COLOR_2 = 0x00000010,  // 2-color cursor
    CURSOR_COLOR_3 = 0x00000020,  // 3-color cursor
    CURSOR_COLOR_16 = 0x00000040, // 16-color cursor
  };

  // Read/write registers
  u32 read(Register reg);
  void write(Register reg, u32 value);

  // Video timing
  void set_timing(u32 h_total, u32 h_display, u32 h_sync_start, u32 h_sync_end,
                  u32 v_total, u32 v_display, u32 v_sync_start, u32 v_sync_end,
                  bool interlaced = false);
  void get_timing(u32 *h_total, u32 *h_display, u32 *h_sync_start,
                  u32 *h_sync_end, u32 *v_total, u32 *v_display,
                  u32 *v_sync_start, u32 *v_sync_end) const;

  // Framebuffer
  void set_framebuffer(u32 phys_addr, u32 stride, u32 width, u32 height,
                       Format fmt);
  void set_framebuffer_offset(int x, int y);

  // Cursor
  void set_cursor_position(int x, int y);
  void set_cursor_hotspot(int x, int y);
  void set_cursor_colors(u32 color1, u32 color2, u32 color3);
  void set_cursor_pattern(const u8 *pattern, int width, int height);
  void enable_cursor(bool enable);

  // Gamma
  void set_gamma(const u16 *red, const u16 *green, const u16 *blue);
  void enable_gamma(bool enable);

  // Tile configuration (GBE)
  void set_tile_config(u32 config);
  void set_tile_framebuffer(u32 phys_addr, u32 stride);

  // Video modes (from IRIX crm_timing.h)
  struct VideoMode {
    const char *name;
    u32 width, height;
    u32 h_total, h_display, h_sync_start, h_sync_end;
    u32 v_total, v_display, v_sync_start, v_sync_end;
    bool interlaced;
    u32 pixel_clock_khz;
  };

  static const VideoMode *get_video_mode(const char *name);
  void set_video_mode(const VideoMode *mode);

  // Status
  bool vblank() const;
  bool hblank() const;
  u32 status() const;
  u32 interrupt_status() const;

  // Reset
  void reset();

private:
  std::array<u32, 0x10000 / 4> regs_ = {}; // 64KB register space

  // State
  bool enabled_ = false;
  u32 control_ = 0;
  u32 status_ = 0;
  u32 interrupt_mask_ = 0;
  u32 interrupt_status_ = 0;

  // Video timing
  u32 h_total_ = 800;
  u32 h_display_ = 640;
  u32 h_sync_start_ = 656;
  u32 h_sync_end_ = 752;
  u32 v_total_ = 525;
  u32 v_display_ = 480;
  u32 v_sync_start_ = 490;
  u32 v_sync_end_ = 492;
  bool interlaced_ = false;

  // Framebuffer
  u32 fb_addr_ = 0;
  u32 fb_stride_ = 0;
  u32 fb_width_ = 640;
  u32 fb_height_ = 480;
  Format fb_format_ = FMT_32BPP;
  int fb_offset_x_ = 0;
  int fb_offset_y_ = 0;

  // Cursor
  int cursor_x_ = 0;
  int cursor_y_ = 0;
  int cursor_hot_x_ = 0;
  int cursor_hot_y_ = 0;
  u32 cursor_color1_ = 0xFFFFFFFF;
  u32 cursor_color2_ = 0xFF000000;
  u32 cursor_color3_ = 0x00000000;
  std::array<u8, 64 * 64 / 4> cursor_pattern_ = {}; // 2bpp
  bool cursor_enabled_ = false;

  // Gamma
  std::array<u16, 256> gamma_red_ = {};
  std::array<u16, 256> gamma_green_ = {};
  std::array<u16, 256> gamma_blue_ = {};
  bool gamma_enabled_ = false;

  // Tile config
  u32 tile_config_ = 0;
  u32 tile_fb_base_ = 0;
  u32 tile_fb_stride_ = 0;

  // Line counter for interrupts
  u32 current_line_ = 0;
  u32 line_compare_ = 0;
};

} // namespace o2emu::graphics