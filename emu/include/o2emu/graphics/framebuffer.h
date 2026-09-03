/**
 * @file framebuffer.h
 * @brief Framebuffer (tile-based GBE) + video modes
 *
 * Based on IRIX crm_fb.h and PROM definitions
 * The GBE (Graphics Back End) handles tile-based framebuffer management
 */

#pragma once

#include <array>
#include <o2emu/o2emu.h>

namespace o2emu::graphics {

class Framebuffer {
public:
  Framebuffer();
  ~Framebuffer() = default;

  // Framebuffer register offsets (from PHYS_BASE_GBE = 0x16000000)
  // Overlaps with Display Engine but focuses on tile management
  enum Register : uint32_t {
    // Tile memory
    FB_TILE_BASE = 0x000000,
    FB_TILE_MEMORY = 0x000000, // Tile memory (4MB)

    // Tile directory
    FB_TILE_DIR_BASE = 0x400000,
    FB_TILE_DIR = 0x400000, // Tile directory (64KB)

    // Tile configuration
    FB_TILE_CONFIG = 0x410000,
    FB_TILE_WIDTH = 0x410004,
    FB_TILE_HEIGHT = 0x410008,
    FB_TILE_BPP = 0x41000C,

    // Framebuffer control
    FB_CONTROL = 0x420000,
    FB_BASE_ADDR = 0x420004,
    FB_STRIDE = 0x420008,
    FB_WIDTH = 0x42000C,
    FB_HEIGHT = 0x420010,
    FB_FORMAT = 0x420014,

    // Double buffering
    FB_FRONT_BASE = 0x420020,
    FB_BACK_BASE = 0x420024,
    FB_SWAP = 0x420028,

    // Status/Control
    FB_STATUS = 0x430000,
    FB_RESET = 0x430004,
    FB_REVISION = 0x430008,
  };

  // Control bits
  enum ControlBit : uint32_t {
    CTRL_ENABLE = 0x00000001,
    CTRL_RESET = 0x00000002,
    CTRL_DOUBLE_BUFFER = 0x00000010,
    CTRL_TILE_MODE = 0x00000100,
    CTRL_LINEAR_MODE = 0x00000200,
    CTRL_SWAP_PENDING = 0x00001000,
  };

  // Status bits
  enum StatusBit : uint32_t {
    STATUS_SWAP_PENDING = 0x00000001,
    STATUS_TILE_DIRTY = 0x00000010,
    STATUS_MEMORY_FULL = 0x00000100,
  };

  // Tile configuration
  enum TileConfig : uint32_t {
    TILE_8x8 = 0x00,   // 8x8 tiles
    TILE_16x16 = 0x01, // 16x16 tiles
    TILE_32x32 = 0x02, // 32x32 tiles
    TILE_64x64 = 0x03, // 64x64 tiles
  };

  // Framebuffer formats
  enum Format : uint32_t {
    FMT_8BPP = 0x00,
    FMT_16BPP = 0x01,
    FMT_24BPP = 0x02,
    FMT_32BPP = 0x03,
  };

  // Read/write registers
  u32 read(Register reg);
  void write(Register reg, u32 value);

  // Framebuffer configuration
  void set_format(Format fmt);
  void set_dimensions(u32 width, u32 height);
  void set_stride(u32 stride);
  void set_base_address(u32 phys_addr);
  void set_tile_config(TileConfig config, u32 bpp);

  // Double buffering
  void set_double_buffer(bool enable);
  void set_front_buffer(u32 phys_addr);
  void set_back_buffer(u32 phys_addr);
  void swap_buffers();
  bool swap_pending() const;

  // Tile memory management
  void write_tile(u32 tile_x, u32 tile_y, const u32 *data, u32 size);
  void read_tile(u32 tile_x, u32 tile_y, u32 *data, u32 size);
  void clear_tile(u32 tile_x, u32 tile_y, u32 color);
  void invalidate_tile(u32 tile_x, u32 tile_y);

  // Linear framebuffer access (for CPU)
  void write_pixel(u32 x, u32 y, u32 color);
  u32 read_pixel(u32 x, u32 y) const;
  void write_span(u32 x, u32 y, u32 width, const u32 *colors);
  void read_span(u32 x, u32 y, u32 width, u32 *colors) const;

  // Clear
  void clear(u32 color);
  void clear_rect(u32 x, u32 y, u32 width, u32 height, u32 color);

  // Blit
  void blit(u32 src_x, u32 src_y, u32 dst_x, u32 dst_y, u32 width, u32 height,
            u32 rop);

  // Status
  u32 status() const;
  u32 width() const { return width_; }
  u32 height() const { return height_; }
  u32 stride() const { return stride_; }
  Format format() const { return format_; }
  u32 base_address() const { return base_addr_; }

  // Video modes (from IRIX crm_timing.h)
  struct VideoMode {
    const char *name;
    u32 width, height;
    u32 h_total, h_display, h_sync_start, h_sync_end;
    u32 v_total, v_display, v_sync_start, v_sync_end;
    bool interlaced;
    u32 pixel_clock_khz;
    u32 refresh_rate_hz;
  };

  static const VideoMode *get_video_mode(const char *name);
  static const VideoMode *get_video_mode_by_resolution(u32 width, u32 height,
                                                       u32 refresh_rate);
  void set_video_mode(const VideoMode *mode);

  // Reset
  void reset();

private:
  std::array<u32, 0x10000 / 4> regs_ = {}; // 64KB register space

  // Tile memory (4MB)
  std::vector<u8> tile_memory_;

  // Tile directory (64KB)
  std::vector<u32> tile_directory_;

  // State
  bool enabled_ = false;
  u32 control_ = 0;
  u32 status_ = 0;

  // Framebuffer config
  Format format_ = FMT_32BPP;
  u32 width_ = 640;
  u32 height_ = 480;
  u32 stride_ = 640 * 4;
  u32 base_addr_ = 0;
  u32 front_base_ = 0;
  u32 back_base_ = 0;
  bool double_buffer_ = false;
  bool swap_pending_ = false;

  // Tile config
  TileConfig tile_config_ = TILE_16x16;
  u32 tile_bpp_ = 32;
  u32 tile_width_ = 16;
  u32 tile_height_ = 16;
  u32 tiles_x_ = 0;
  u32 tiles_y_ = 0;

  // Current buffer
  u32 current_base_ = 0;
};

} // namespace o2emu::graphics