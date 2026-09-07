/**
 * @file gbe_framebuffer.cpp
 * @brief GBE Framebuffer Plane Registers Implementation
 *
 * Implements the GBE framebuffer plane registers at GBE_BASE + 0x30000.
 */

#include <o2emu/graphics/gbe_framebuffer.h>
#include <o2emu/logging/logger.h>

namespace o2emu::graphics {

GBEFramebuffer::GBEFramebuffer()
    : devices::Device("GBE Framebuffer", 0x16030000, 0x10000) {
  // GBE framebuffer plane registers at GBE_BASE + 0x30000
  // Size 0x10000 (64KB) to cover the register space
  reset();
}

u32 GBEFramebuffer::read32(u32 offset) {
  switch (offset) {
  case FRM_SIZE_TILE:
    return frm_size_tile_;
  case FRM_SIZE_PIXEL:
    return frm_size_pixel_;
  case FRM_INHWCTRL:
    return frm_inhwctrl_;
  case FRM_CONTROL:
    return frm_control_;
  default:
    O2EMU_LOG_WARN_F("GBEFramebuffer: read32 from unknown offset 0x{:08X}",
                     offset);
    return 0;
  }
}

u16 GBEFramebuffer::read16(u32 offset) {
  return static_cast<u16>(read32(offset) >> ((offset & 2) * 8));
}

u8 GBEFramebuffer::read8(u32 offset) {
  return static_cast<u8>(read32(offset) >> ((offset & 3) * 8));
}

void GBEFramebuffer::write32(u32 offset, u32 value) {
  switch (offset) {
  case FRM_SIZE_TILE:
    frm_size_tile_ = value;
    update_derived_params();
    O2EMU_LOG_DEBUG_F("GBEFramebuffer: FRM_SIZE_TILE = 0x{:08X}", value);
    break;
  case FRM_SIZE_PIXEL:
    frm_size_pixel_ = value;
    update_derived_params();
    O2EMU_LOG_DEBUG_F("GBEFramebuffer: FRM_SIZE_PIXEL = 0x{:08X}", value);
    break;
  case FRM_INHWCTRL:
    frm_inhwctrl_ = value;
    O2EMU_LOG_DEBUG_F("GBEFramebuffer: FRM_INHWCTRL = 0x{:08X}", value);
    break;
  case FRM_CONTROL:
    frm_control_ = value;
    update_derived_params();
    O2EMU_LOG_DEBUG_F("GBEFramebuffer: FRM_CONTROL = 0x{:08X}", value);
    break;
  default:
    O2EMU_LOG_WARN_F(
        "GBEFramebuffer: write32 to unknown offset 0x{:08X} = 0x{:08X}", offset,
        value);
    break;
  }
}

void GBEFramebuffer::write16(u32 offset, u16 value) {
  u32 current = read32(offset);
  if (offset & 2) {
    current = (current & 0x0000FFFF) | (static_cast<u32>(value) << 16);
  } else {
    current = (current & 0xFFFF0000) | value;
  }
  write32(offset, current);
}

void GBEFramebuffer::write8(u32 offset, u8 value) {
  u32 current = read32(offset);
  u32 shift = (offset & 3) * 8;
  current = (current & ~(0xFF << shift)) | (static_cast<u32>(value) << shift);
  write32(offset, current);
}

void GBEFramebuffer::reset() {
  frm_size_tile_ = 0;
  frm_size_pixel_ = 0;
  frm_inhwctrl_ = 0;
  frm_control_ = 0;
  update_derived_params();
}

void GBEFramebuffer::update_derived_params() {
  // Extract framebuffer parameters from registers
  // FRM_SIZE_TILE (0x30000)
  u32 width_tiles = (frm_size_tile_ & FRM_WIDTH_TILE_MASK) >> 5;
  u32 depth_code = (frm_size_tile_ & FRM_DEPTH_MASK) >> FRM_DEPTH_SHIFT;

  // FRM_SIZE_PIXEL (0x30004)
  u32 height_pixels = (frm_size_pixel_ & FB_HEIGHT_PIX_MASK) >> 16;
  linear_mode_ = (frm_control_ & FRM_LINEAR) != 0;
  tile_ptr_ = (frm_control_ & FRM_TILE_PTR_MASK) >> 9;

  // Convert depth code to bits per pixel
  // Depth codes from IRIX/Linux sources:
  // 0 = 8bpp, 1 = 16bpp, 2 = 32bpp, 3 = 32bpp/reserved
  static constexpr u32 depth_to_bpp[4] = {8, 16, 32, 32};
  fb_depth_ = (depth_code < 4) ? depth_to_bpp[depth_code] : 8;

  // Normal tiles are 512 bytes wide, so their pixel width depends on depth.
  const u32 bytes_per_pixel = fb_depth_ / 8;
  if (width_tiles > 0) {
    fb_width_ = width_tiles * (512 / bytes_per_pixel);
  } else {
    fb_width_ = 1280; // Default
  }

  fb_height_ = height_pixels > 0 ? height_pixels : 1024;

  // Calculate stride (bytes per row)
  fb_stride_ = (fb_width_ * fb_depth_) / 8;

  fb_base_ = tile_ptr_ << 9;

  O2EMU_LOG_INFO_F(
      "GBEFramebuffer: {}x{} @ {}bpp, stride={}, linear={}, tile_ptr={}, "
      "base=0x{:08X}",
      fb_width_, fb_height_, fb_depth_, fb_stride_, linear_mode_, tile_ptr_,
      fb_base_);
}

} // namespace o2emu::graphics