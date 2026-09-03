/**
 * @file display_engine.cpp
 * @brief Display Engine (analog video out) implementation
 */

#include <cstring>
#include <o2emu/graphics/display_engine.h>
#include <o2emu/logging/logger.h>

namespace o2emu::graphics {

DisplayEngine::DisplayEngine() { reset(); }

DisplayEngine::~DisplayEngine() = default;

u32 DisplayEngine::read(Register reg) {
  switch (reg) {
  case DE_CONTROL:
    return control_;

  case DE_STATUS:
    return status_;

  case DE_INTERRUPT:
    return interrupt_status_;

  case DE_INTERRUPT_MASK:
    return interrupt_mask_;

  case DE_H_TOTAL:
    return h_total_;
  case DE_H_DISPLAY:
    return h_display_;
  case DE_H_SYNC_START:
    return h_sync_start_;
  case DE_H_SYNC_END:
    return h_sync_end_;
  case DE_V_TOTAL:
    return v_total_;
  case DE_V_DISPLAY:
    return v_display_;
  case DE_V_SYNC_START:
    return v_sync_start_;
  case DE_V_SYNC_END:
    return v_sync_end_;
  case DE_V_SYNC_START_ODD:
    return v_sync_start_;
  case DE_V_SYNC_END_ODD:
    return v_sync_end_;

  case DE_FB_ADDR:
    return fb_addr_;
  case DE_FB_STRIDE:
    return fb_stride_;
  case DE_FB_FORMAT:
    return static_cast<u32>(fb_format_);
  case DE_FB_WIDTH:
    return fb_width_;
  case DE_FB_HEIGHT:
    return fb_height_;
  case DE_FB_OFFSET_X:
    return static_cast<u32>(fb_offset_x_);
  case DE_FB_OFFSET_Y:
    return static_cast<u32>(fb_offset_y_);

  case DE_CURSOR_CONTROL:
    return regs_[DE_CURSOR_CONTROL];
  case DE_CURSOR_POS_X:
    return static_cast<u32>(cursor_x_);
  case DE_CURSOR_POS_Y:
    return static_cast<u32>(cursor_y_);
  case DE_CURSOR_HOT_X:
    return static_cast<u32>(cursor_hot_x_);
  case DE_CURSOR_HOT_Y:
    return static_cast<u32>(cursor_hot_y_);
  case DE_CURSOR_COLOR_1:
    return cursor_color1_;
  case DE_CURSOR_COLOR_2:
    return cursor_color2_;
  case DE_CURSOR_COLOR_3:
    return cursor_color3_;

  case DE_DAC_CONTROL:
    return regs_[DE_DAC_CONTROL];

  case DE_TILE_CONFIG:
    return tile_config_;
  case DE_TILE_FB_BASE:
    return tile_fb_base_;
  case DE_TILE_FB_STRIDE:
    return tile_fb_stride_;

  case DE_REVISION:
    return 0x00010000; // Version 1.0

  default:
    O2EMU_LOG_DEBUG("DisplayEngine read from unknown register: 0x"
                    << std::hex << reg << std::dec);
    return regs_[reg];
  }
}

void DisplayEngine::write(Register reg, u32 value) {
  switch (reg) {
  case DE_CONTROL:
    control_ = value;
    enabled_ = (value & CTRL_ENABLE) != 0;
    if (value & CTRL_RESET) {
      reset();
    }
    break;

  case DE_INTERRUPT_MASK:
    interrupt_mask_ = value;
    break;

  case DE_H_TOTAL:
    h_total_ = value;
    break;
  case DE_H_DISPLAY:
    h_display_ = value;
    break;
  case DE_H_SYNC_START:
    h_sync_start_ = value;
    break;
  case DE_H_SYNC_END:
    h_sync_end_ = value;
    break;
  case DE_V_TOTAL:
    v_total_ = value;
    break;
  case DE_V_DISPLAY:
    v_display_ = value;
    break;
  case DE_V_SYNC_START:
    v_sync_start_ = value;
    break;
  case DE_V_SYNC_END:
    v_sync_end_ = value;
    break;
  case DE_V_SYNC_START_ODD:
    // For interlaced
    break;
  case DE_V_SYNC_END_ODD:
    // For interlaced
    break;

  case DE_FB_ADDR:
    fb_addr_ = value;
    current_base_ = value;
    break;
  case DE_FB_STRIDE:
    fb_stride_ = value;
    break;
  case DE_FB_FORMAT:
    fb_format_ = static_cast<Format>(value & 0xF);
    break;
  case DE_FB_WIDTH:
    fb_width_ = value;
    break;
  case DE_FB_HEIGHT:
    fb_height_ = value;
    break;
  case DE_FB_OFFSET_X:
    fb_offset_x_ = static_cast<int>(value);
    break;
  case DE_FB_OFFSET_Y:
    fb_offset_y_ = static_cast<int>(value);
    break;

  case DE_CURSOR_CONTROL:
    regs_[DE_CURSOR_CONTROL] = value;
    cursor_enabled_ = (value & CURSOR_ENABLE) != 0;
    break;
  case DE_CURSOR_POS_X:
    cursor_x_ = static_cast<int>(value);
    break;
  case DE_CURSOR_POS_Y:
    cursor_y_ = static_cast<int>(value);
    break;
  case DE_CURSOR_HOT_X:
    cursor_hot_x_ = static_cast<int>(value);
    break;
  case DE_CURSOR_HOT_Y:
    cursor_hot_y_ = static_cast<int>(value);
    break;
  case DE_CURSOR_COLOR_1:
    cursor_color1_ = value;
    break;
  case DE_CURSOR_COLOR_2:
    cursor_color2_ = value;
    break;
  case DE_CURSOR_COLOR_3:
    cursor_color3_ = value;
    break;
  case DE_CURSOR_PATTERN:
    // Cursor pattern - 64x64 2bpp = 1024 bytes
    break;

  case DE_DAC_CONTROL:
    regs_[DE_DAC_CONTROL] = value;
    break;

  case DE_GAMMA_RED:
  case DE_GAMMA_GREEN:
  case DE_GAMMA_BLUE:
    // Gamma tables - 256 entries each
    break;

  case DE_TILE_CONFIG:
    tile_config_ = value;
    break;
  case DE_TILE_FB_BASE:
    tile_fb_base_ = value;
    break;
  case DE_TILE_FB_STRIDE:
    tile_fb_stride_ = value;
    break;

  case DE_RESET:
    if (value & 0x1) {
      reset();
    }
    break;

  default:
    O2EMU_LOG_DEBUG("DisplayEngine write to unknown register: 0x"
                    << std::hex << reg << std::dec << " = 0x" << value);
    regs_[reg] = value;
    break;
  }
}

void DisplayEngine::set_timing(u32 h_total, u32 h_display, u32 h_sync_start,
                               u32 h_sync_end, u32 v_total, u32 v_display,
                               u32 v_sync_start, u32 v_sync_end,
                               bool interlaced) {
  h_total_ = h_total;
  h_display_ = h_display;
  h_sync_start_ = h_sync_start;
  h_sync_end_ = h_sync_end;
  v_total_ = v_total;
  v_display_ = v_display;
  v_sync_start_ = v_sync_start;
  v_sync_end_ = v_sync_end;
  interlaced_ = interlaced;

  if (interlaced) {
    control_ |= CTRL_INTERLACE;
  } else {
    control_ &= ~CTRL_INTERLACE;
  }

  O2EMU_LOG_DEBUG("DisplayEngine timing: " << h_display << "x" << v_display
                                           << "@" << (h_total * v_total)
                                           << "Hz");
}

void DisplayEngine::get_timing(u32 *h_total, u32 *h_display, u32 *h_sync_start,
                               u32 *h_sync_end, u32 *v_total, u32 *v_display,
                               u32 *v_sync_start, u32 *v_sync_end) const {
  if (h_total)
    *h_total = h_total_;
  if (h_display)
    *h_display = h_display_;
  if (h_sync_start)
    *h_sync_start = h_sync_start_;
  if (h_sync_end)
    *h_sync_end = h_sync_end_;
  if (v_total)
    *v_total = v_total_;
  if (v_display)
    *v_display = v_display_;
  if (v_sync_start)
    *v_sync_start = v_sync_start_;
  if (v_sync_end)
    *v_sync_end = v_sync_end_;
}

void DisplayEngine::set_framebuffer(u32 phys_addr, u32 stride, u32 width,
                                    u32 height, Format fmt) {
  fb_addr_ = phys_addr;
  fb_stride_ = stride;
  fb_width_ = width;
  fb_height_ = height;
  fb_format_ = fmt;
  current_base_ = phys_addr;

  O2EMU_LOG_DEBUG("DisplayEngine framebuffer: " << width << "x" << height
                                                << " stride=" << stride
                                                << " fmt=" << fmt);
}

void DisplayEngine::set_framebuffer_offset(int x, int y) {
  fb_offset_x_ = x;
  fb_offset_y_ = y;
}

void DisplayEngine::set_cursor_position(int x, int y) {
  cursor_x_ = x;
  cursor_y_ = y;
}

void DisplayEngine::set_cursor_hotspot(int x, int y) {
  cursor_hot_x_ = x;
  cursor_hot_y_ = y;
}

void DisplayEngine::set_cursor_colors(u32 color1, u32 color2, u32 color3) {
  cursor_color1_ = color1;
  cursor_color2_ = color2;
  cursor_color3_ = color3;
}

void DisplayEngine::set_cursor_pattern(const u8 *pattern, int width,
                                       int height) {
  if (pattern && width <= 64 && height <= 64) {
    // Pattern is 2bpp, so each byte holds 4 pixels
    int pattern_size = (width * height + 3) / 4;
    std::memcpy(cursor_pattern_.data(), pattern, pattern_size);
  }
}

void DisplayEngine::enable_cursor(bool enable) {
  cursor_enabled_ = enable;
  if (enable) {
    control_ |= CTRL_CURSOR_ENABLE;
  } else {
    control_ &= ~CTRL_CURSOR_ENABLE;
  }
}

void DisplayEngine::set_gamma(const u16 *red, const u16 *green,
                              const u16 *blue) {
  if (red)
    std::memcpy(gamma_red_.data(), red, 256 * sizeof(u16));
  if (green)
    std::memcpy(gamma_green_.data(), green, 256 * sizeof(u16));
  if (blue)
    std::memcpy(gamma_blue_.data(), blue, 256 * sizeof(u16));
}

void DisplayEngine::enable_gamma(bool enable) {
  gamma_enabled_ = enable;
  if (enable) {
    control_ |= CTRL_GAMMA_ENABLE;
  } else {
    control_ &= ~CTRL_GAMMA_ENABLE;
  }
}

void DisplayEngine::set_tile_config(u32 config) { tile_config_ = config; }

void DisplayEngine::set_tile_framebuffer(u32 phys_addr, u32 stride) {
  tile_fb_base_ = phys_addr;
  tile_fb_stride_ = stride;
}

const DisplayEngine::VideoMode *
DisplayEngine::get_video_mode(const char *name) {
  static const VideoMode modes[] = {
      {"640x480@60", 640, 480, 800, 640, 656, 752, 525, 480, 490, 492, false,
       25175, 60},
      {"800x600@60", 800, 600, 1056, 800, 840, 968, 628, 600, 601, 605, false,
       40000, 60},
      {"1024x768@60", 1024, 768, 1344, 1024, 1048, 1184, 806, 768, 771, 777,
       false, 65000, 60},
      {"1280x1024@60", 1280, 1024, 1688, 1280, 1328, 1440, 1066, 1024, 1025,
       1028, false, 108000, 60},
      {"1600x1200@60", 1600, 1200, 2160, 1600, 1664, 1856, 1250, 1200, 1201,
       1204, false, 162000, 60},
      {"640x480@72", 640, 480, 832, 640, 656, 752, 520, 480, 490, 492, false,
       31500, 72},
      {"800x600@72", 800, 600, 1040, 800, 824, 968, 666, 600, 601, 605, false,
       50000, 72},
      {"1024x768@70", 1024, 768, 1328, 1024, 1048, 1184, 806, 768, 771, 777,
       false, 75000, 70},
      {"1280x1024@75", 1280, 1024, 1688, 1280, 1328, 1440, 1066, 1024, 1025,
       1028, false, 135000, 75},
      {"1600x1200@65", 1600, 1200, 2160, 1600, 1664, 1856, 1250, 1200, 1201,
       1204, false, 175500, 65},
      {nullptr, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, false, 0, 0}};

  for (const auto &mode : modes) {
    if (mode.name && std::strcmp(mode.name, name) == 0) {
      return &mode;
    }
  }
  return nullptr;
}

const DisplayEngine::VideoMode *
DisplayEngine::get_video_mode_by_resolution(u32 width, u32 height,
                                            u32 refresh_rate) {
  static const VideoMode modes[] = {
      {"640x480@60", 640, 480, 800, 640, 656, 752, 525, 480, 490, 492, false,
       25175, 60},
      {"800x600@60", 800, 600, 1056, 800, 840, 968, 628, 600, 601, 605, false,
       40000, 60},
      {"1024x768@60", 1024, 768, 1344, 1024, 1048, 1184, 806, 768, 771, 777,
       false, 65000, 60},
      {"1280x1024@60", 1280, 1024, 1688, 1280, 1328, 1440, 1066, 1024, 1025,
       1028, false, 108000, 60},
      {"1600x1200@60", 1600, 1200, 2160, 1600, 1664, 1856, 1250, 1200, 1201,
       1204, false, 162000, 60},
      {"640x480@72", 640, 480, 832, 640, 656, 752, 520, 480, 490, 492, false,
       31500, 72},
      {"800x600@72", 800, 600, 1040, 800, 824, 968, 666, 600, 601, 605, false,
       50000, 72},
      {"1024x768@70", 1024, 768, 1328, 1024, 1048, 1184, 806, 768, 771, 777,
       false, 75000, 70},
      {"1280x1024@75", 1280, 1024, 1688, 1280, 1328, 1440, 1066, 1024, 1025,
       1028, false, 135000, 75},
      {"1600x1200@65", 1600, 1200, 2160, 1600, 1664, 1856, 1250, 1200, 1201,
       1204, false, 175500, 65},
      {nullptr, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, false, 0, 0}};

  for (const auto &mode : modes) {
    if (mode.name && mode.width == width && mode.height == height &&
        mode.refresh_rate_hz == refresh_rate) {
      return &mode;
    }
  }
  return nullptr;
}

void DisplayEngine::set_video_mode(const VideoMode *mode) {
  if (!mode)
    return;

  set_timing(mode->h_total, mode->h_display, mode->h_sync_start,
             mode->h_sync_end, mode->v_total, mode->v_display,
             mode->v_sync_start, mode->v_sync_end, mode->interlaced);

  fb_width_ = mode->width;
  fb_height_ = mode->height;

  O2EMU_LOG_DEBUG("DisplayEngine video mode: " << mode->name);
}

bool DisplayEngine::vblank() const { return (status_ & STATUS_VBLANK) != 0; }

bool DisplayEngine::hblank() const { return (status_ & STATUS_HBLANK) != 0; }

u32 DisplayEngine::status() const { return status_; }

u32 DisplayEngine::interrupt_status() const {
  return interrupt_status_ & interrupt_mask_;
}

void DisplayEngine::reset() {
  std::memset(regs_.data(), 0, regs_.size() * sizeof(u32));
  enabled_ = false;
  control_ = 0;
  status_ = 0;
  interrupt_mask_ = 0;
  interrupt_status_ = 0;

  // Default timing (640x480@60)
  h_total_ = 800;
  h_display_ = 640;
  h_sync_start_ = 656;
  h_sync_end_ = 752;
  v_total_ = 525;
  v_display_ = 480;
  v_sync_start_ = 490;
  v_sync_end_ = 492;
  interlaced_ = false;

  fb_addr_ = 0;
  fb_stride_ = 640 * 4;
  fb_width_ = 640;
  fb_height_ = 480;
  fb_format_ = FMT_32BPP;
  fb_offset_x_ = 0;
  fb_offset_y_ = 0;
  current_base_ = 0;

  cursor_x_ = 0;
  cursor_y_ = 0;
  cursor_hot_x_ = 0;
  cursor_hot_y_ = 0;
  cursor_color1_ = 0xFFFFFFFF;
  cursor_color2_ = 0xFF000000;
  cursor_color3_ = 0x00000000;
  cursor_pattern_.fill(0);
  cursor_enabled_ = false;

  gamma_red_.fill(0);
  gamma_green_.fill(0);
  gamma_blue_.fill(0);
  for (int i = 0; i < 256; i++) {
    gamma_red_[i] = static_cast<u16>(i << 8);
    gamma_green_[i] = static_cast<u16>(i << 8);
    gamma_blue_[i] = static_cast<u16>(i << 8);
  }
  gamma_enabled_ = false;

  tile_config_ = 0;
  tile_fb_base_ = 0;
  tile_fb_stride_ = 0;

  current_line_ = 0;
  line_compare_ = 0;
}

} // namespace o2emu::graphics