/**
 * @file framebuffer.cpp
 * @brief Framebuffer (tile-based GBE) implementation
 */

#include <algorithm>
#include <cstring>
#include <o2emu/graphics/framebuffer.h>
#include <o2emu/logging/logger.h>
#include <vector>

namespace o2emu::graphics {

Framebuffer::Framebuffer() { reset(); }

Framebuffer::~Framebuffer() = default;

u32 Framebuffer::read(Register reg) {
  switch (reg) {
  case FB_CONTROL:
    return control_;

  case FB_BASE_ADDR:
    return base_addr_;

  case FB_STRIDE:
    return stride_;

  case FB_WIDTH:
    return width_;

  case FB_HEIGHT:
    return height_;

  case FB_FORMAT:
    return static_cast<u32>(format_);

  case FB_FRONT_BASE:
    return front_base_;

  case FB_BACK_BASE:
    return back_base_;

  case FB_STATUS:
    return status_;

  case FB_REVISION:
    return 0x00010000; // Version 1.0

  default:
    // Tile memory and directory reads
    if (reg >= FB_TILE_BASE && reg < FB_TILE_DIR_BASE) {
      u32 offset = reg - FB_TILE_BASE;
      if (offset < tile_memory_.size()) {
        return *reinterpret_cast<u32 *>(&tile_memory_[offset]);
      }
    } else if (reg >= FB_TILE_DIR_BASE && reg < FB_TILE_CONFIG) {
      u32 offset = (reg - FB_TILE_DIR_BASE) / 4;
      if (offset < tile_directory_.size()) {
        return tile_directory_[offset];
      }
    }
    O2EMU_LOG_DEBUG("Framebuffer read from unknown register: 0x"
                    << std::hex << reg << std::dec);
    return 0;
  }
}

void Framebuffer::write(Register reg, u32 value) {
  switch (reg) {
  case FB_CONTROL:
    control_ = value;
    enabled_ = (value & CTRL_ENABLE) != 0;
    double_buffer_ = (value & CTRL_DOUBLE_BUFFER) != 0;
    if (value & CTRL_RESET) {
      reset();
    }
    if (value & CTRL_SWAP_PENDING) {
      swap_buffers();
    }
    break;

  case FB_BASE_ADDR:
    base_addr_ = value;
    current_base_ = value;
    break;

  case FB_STRIDE:
    stride_ = value;
    break;

  case FB_WIDTH:
    width_ = value;
    break;

  case FB_HEIGHT:
    height_ = value;
    break;

  case FB_FORMAT:
    format_ = static_cast<Format>(value & 0xF);
    break;

  case FB_FRONT_BASE:
    front_base_ = value;
    if (!double_buffer_) {
      current_base_ = value;
    }
    break;

  case FB_BACK_BASE:
    back_base_ = value;
    break;

  case FB_SWAP:
    swap_buffers();
    break;

  case FB_TILE_CONFIG:
    tile_config_ = static_cast<TileConfig>(value & 0x3);
    update_tile_dimensions();
    break;

  case FB_TILE_WIDTH:
    tile_width_ = value;
    break;

  case FB_TILE_HEIGHT:
    tile_height_ = value;
    break;

  case FB_TILE_BPP:
    tile_bpp_ = value;
    break;

  case FB_RESET:
    if (value & 0x1) {
      reset();
    }
    break;

  default:
    // Tile memory and directory writes
    if (reg >= FB_TILE_BASE && reg < FB_TILE_DIR_BASE) {
      u32 offset = reg - FB_TILE_BASE;
      if (offset + 4 <= tile_memory_.size()) {
        *reinterpret_cast<u32 *>(&tile_memory_[offset]) = value;
      }
    } else if (reg >= FB_TILE_DIR_BASE && reg < FB_TILE_CONFIG) {
      u32 offset = (reg - FB_TILE_DIR_BASE) / 4;
      if (offset < tile_directory_.size()) {
        tile_directory_[offset] = value;
      }
    } else {
      O2EMU_LOG_DEBUG("Framebuffer write to unknown register: 0x"
                      << std::hex << reg << std::dec << " = 0x" << value);
    }
    break;
  }
}

void Framebuffer::set_format(Format fmt) {
  format_ = fmt;
  update_stride();
}

void Framebuffer::set_dimensions(u32 width, u32 height) {
  width_ = width;
  height_ = height;
  update_stride();
  update_tile_dimensions();
}

void Framebuffer::set_stride(u32 stride) { stride_ = stride; }

void Framebuffer::set_base_address(u32 phys_addr) {
  base_addr_ = phys_addr;
  current_base_ = phys_addr;
}

void Framebuffer::set_tile_config(TileConfig config, u32 bpp) {
  tile_config_ = config;
  tile_bpp_ = bpp;
  update_tile_dimensions();
}

void Framebuffer::set_double_buffer(bool enable) {
  double_buffer_ = enable;
  if (enable) {
    control_ |= CTRL_DOUBLE_BUFFER;
  } else {
    control_ &= ~CTRL_DOUBLE_BUFFER;
  }
}

void Framebuffer::set_front_buffer(u32 phys_addr) {
  front_base_ = phys_addr;
  if (!double_buffer_) {
    current_base_ = phys_addr;
  }
}

void Framebuffer::set_back_buffer(u32 phys_addr) { back_base_ = phys_addr; }

void Framebuffer::swap_buffers() {
  if (double_buffer_) {
    std::swap(front_base_, back_base_);
    current_base_ = front_base_;
    swap_pending_ = true;
    status_ |= STATUS_SWAP_PENDING;
  }
}

bool Framebuffer::swap_pending() const { return swap_pending_; }

void Framebuffer::write_tile(u32 tile_x, u32 tile_y, const u32 *data,
                             u32 size) {
  if (tile_x >= tiles_x_ || tile_y >= tiles_y_)
    return;

  u32 tile_index = tile_y * tiles_x_ + tile_x;
  u32 tile_offset = tile_index * tile_width_ * tile_height_ * (tile_bpp_ / 8);

  if (tile_offset + size <= tile_memory_.size()) {
    std::memcpy(&tile_memory_[tile_offset], data, size);
    tile_directory_[tile_index] = tile_offset | 0x80000000; // Valid bit
    status_ |= STATUS_TILE_DIRTY;
  }
}

void Framebuffer::read_tile(u32 tile_x, u32 tile_y, u32 *data, u32 size) {
  if (tile_x >= tiles_x_ || tile_y >= tiles_y_)
    return;

  u32 tile_index = tile_y * tiles_x_ + tile_x;
  u32 tile_offset = tile_directory_[tile_index] & 0x7FFFFFFF;

  if (tile_offset + size <= tile_memory_.size()) {
    std::memcpy(data, &tile_memory_[tile_offset], size);
  }
}

void Framebuffer::clear_tile(u32 tile_x, u32 tile_y, u32 color) {
  if (tile_x >= tiles_x_ || tile_y >= tiles_y_)
    return;

  u32 tile_index = tile_y * tiles_x_ + tile_x;
  u32 tile_offset = tile_index * tile_width_ * tile_height_ * (tile_bpp_ / 8);
  u32 tile_size = tile_width_ * tile_height_ * (tile_bpp_ / 8);

  if (tile_offset + tile_size <= tile_memory_.size()) {
    u32 *tile_data = reinterpret_cast<u32 *>(&tile_memory_[tile_offset]);
    u32 num_pixels = tile_width_ * tile_height_;
    for (u32 i = 0; i < num_pixels; i++) {
      tile_data[i] = color;
    }
    tile_directory_[tile_index] = tile_offset | 0x80000000;
    status_ |= STATUS_TILE_DIRTY;
  }
}

void Framebuffer::invalidate_tile(u32 tile_x, u32 tile_y) {
  if (tile_x >= tiles_x_ || tile_y >= tiles_y_)
    return;

  u32 tile_index = tile_y * tiles_x_ + tile_x;
  tile_directory_[tile_index] = 0; // Invalid
}

void Framebuffer::write_pixel(u32 x, u32 y, u32 color) {
  if (x >= width_ || y >= height_)
    return;

  u32 offset = y * stride_ + x * (bytes_per_pixel());
  if (offset + 4 <= tile_memory_.size()) {
    *reinterpret_cast<u32 *>(&tile_memory_[offset]) = color;
  }
}

u32 Framebuffer::read_pixel(u32 x, u32 y) const {
  if (x >= width_ || y >= height_)
    return 0;

  u32 offset = y * stride_ + x * (bytes_per_pixel());
  if (offset + 4 <= tile_memory_.size()) {
    return *reinterpret_cast<const u32 *>(&tile_memory_[offset]);
  }
  return 0;
}

void Framebuffer::write_span(u32 x, u32 y, u32 width, const u32 *colors) {
  if (y >= height_ || x + width > width_)
    return;

  u32 offset = y * stride_ + x * (bytes_per_pixel());
  u32 span_size = width * (bytes_per_pixel());

  if (offset + span_size <= tile_memory_.size()) {
    std::memcpy(&tile_memory_[offset], colors, span_size);
  }
}

void Framebuffer::read_span(u32 x, u32 y, u32 width, u32 *colors) const {
  if (y >= height_ || x + width > width_)
    return;

  u32 offset = y * stride_ + x * (bytes_per_pixel());
  u32 span_size = width * (bytes_per_pixel());

  if (offset + span_size <= tile_memory_.size()) {
    std::memcpy(colors, &tile_memory_[offset], span_size);
  }
}

void Framebuffer::clear(u32 color) { clear_rect(0, 0, width_, height_, color); }

void Framebuffer::clear_rect(u32 x, u32 y, u32 width, u32 height, u32 color) {
  u32 end_x = std::min(x + width, width_);
  u32 end_y = std::min(y + height, height_);
  u32 bpp = bytes_per_pixel();

  for (u32 row = y; row < end_y; row++) {
    u32 offset = row * stride_ + x * bpp;
    u32 row_size = (end_x - x) * bpp;

    if (offset + row_size <= tile_memory_.size()) {
      u32 *row_data = reinterpret_cast<u32 *>(&tile_memory_[offset]);
      for (u32 i = 0; i < (end_x - x); i++) {
        row_data[i] = color;
      }
    }
  }
}

void Framebuffer::blit(u32 src_x, u32 src_y, u32 dst_x, u32 dst_y, u32 width,
                       u32 height, u32 rop) {
  // Simple blit - copy pixels
  u32 bpp = bytes_per_pixel();

  for (u32 row = 0; row < height; row++) {
    u32 src_row = src_y + row;
    u32 dst_row = dst_y + row;

    if (src_row >= height_ || dst_row >= height_)
      continue;

    u32 src_offset = src_row * stride_ + src_x * bpp;
    u32 dst_offset = dst_row * stride_ + dst_x * bpp;
    u32 row_size = width * bpp;

    if (src_offset + row_size <= tile_memory_.size() &&
        dst_offset + row_size <= tile_memory_.size()) {
      std::memcpy(&tile_memory_[dst_offset], &tile_memory_[src_offset],
                  row_size);
    }
  }
}

u32 Framebuffer::status() const { return status_; }

const Framebuffer::VideoMode *Framebuffer::get_video_mode(const char *name) {
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

const Framebuffer::VideoMode *
Framebuffer::get_video_mode_by_resolution(u32 width, u32 height,
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

void Framebuffer::set_video_mode(const VideoMode *mode) {
  if (!mode)
    return;

  width_ = mode->width;
  height_ = mode->height;
  update_stride();
  update_tile_dimensions();

  O2EMU_LOG_DEBUG("Framebuffer video mode: " << mode->name);
}

void Framebuffer::reset() {
  std::memset(regs_.data(), 0, regs_.size() * sizeof(u32));
  enabled_ = false;
  control_ = 0;
  status_ = 0;

  format_ = FMT_32BPP;
  width_ = 640;
  height_ = 480;
  stride_ = 640 * 4;
  base_addr_ = 0;
  front_base_ = 0;
  back_base_ = 0;
  current_base_ = 0;
  double_buffer_ = false;
  swap_pending_ = false;

  tile_config_ = TILE_16x16;
  tile_bpp_ = 32;
  tile_width_ = 16;
  tile_height_ = 16;
  tiles_x_ = 0;
  tiles_y_ = 0;

  // Allocate tile memory (4MB)
  tile_memory_.assign(4 * 1024 * 1024, 0);

  // Allocate tile directory (64KB = 16K entries)
  tile_directory_.assign(16384, 0);

  update_tile_dimensions();
}

void Framebuffer::update_stride() { stride_ = width_ * bytes_per_pixel(); }

void Framebuffer::update_tile_dimensions() {
  switch (tile_config_) {
  case TILE_8x8:
    tile_width_ = 8;
    tile_height_ = 8;
    break;
  case TILE_16x16:
    tile_width_ = 16;
    tile_height_ = 16;
    break;
  case TILE_32x32:
    tile_width_ = 32;
    tile_height_ = 32;
    break;
  case TILE_64x64:
    tile_width_ = 64;
    tile_height_ = 64;
    break;
  }

  tiles_x_ = (width_ + tile_width_ - 1) / tile_width_;
  tiles_y_ = (height_ + tile_height_ - 1) / tile_height_;
}

u32 Framebuffer::bytes_per_pixel() const {
  switch (format_) {
  case FMT_8BPP:
    return 1;
  case FMT_16BPP:
    return 2;
  case FMT_24BPP:
    return 3;
  case FMT_32BPP:
    return 4;
  default:
    return 4;
  }
}

} // namespace o2emu::graphics