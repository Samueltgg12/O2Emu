#pragma once

/**
 * @file gbe_framebuffer.h
 * @brief GBE Framebuffer Plane Registers
 *
 * These registers control the GBE (Graphics Back End) framebuffer planes.
 * Located at GBE_BASE + 0x30000 (0x16030000).
 *
 * Sources:
 * - Linux kernel: drivers/video/crmfb.c
 * - NetBSD: sys/arch/sgimips/dev/crmfb.c
 * - IRIX source: stand/arcs/ip32/gbe.h
 */

#include <o2emu/devices/device.h>
#include <o2emu/o2emu.h>

namespace o2emu::graphics {

class GBEFramebuffer : public devices::Device {
public:
  GBEFramebuffer();
  ~GBEFramebuffer() override = default;

  // Device interface
  u32 read32(u32 offset) override;
  u16 read16(u32 offset) override;
  u8 read8(u32 offset) override;

  void write32(u32 offset, u32 value) override;
  void write16(u32 offset, u16 value) override;
  void write8(u32 offset, u8 value) override;

  void reset() override;

  // Register offsets (relative to GBE_BASE + 0x30000)
  enum Register : u32 {
    FRM_SIZE_TILE = 0x00000,  // Framebuffer size in tiles
    FRM_SIZE_PIXEL = 0x00004, // Framebuffer size in pixels
    FRM_INHWCTRL = 0x00008,   // Hardware control
    FRM_CONTROL = 0x0000C,    // Framebuffer control
  };

  // FRM_SIZE_TILE bitfields (0x30000)
  static constexpr u32 FRM_WIDTH_TILE_MASK =
      0x00001FE0;                                   // bits 5-12: width in tiles
  static constexpr u32 FRM_DEPTH_MASK = 0x00006000; // bits 13-14: depth
  static constexpr u32 FRM_DEPTH_SHIFT = 13;

  // FRM_SIZE_PIXEL bitfields (0x30004)
  static constexpr u32 FB_HEIGHT_PIX_MASK =
      0xFFFF0000; // bits 16-31: height in pixels

  // FRM_CONTROL bitfields (0x3000C)
  static constexpr u32 FRM_LINEAR = 0x00000002;
  static constexpr u32 FRM_TILE_PTR_MASK = 0xFFFFFE00;

  // FRM_INHWCTRL bitfields (0x30008)
  static constexpr u32 FRM_HWCTRL_ENABLE =
      0x00000001; // bit 0: hardware control enable
  static constexpr u32 FRM_HWCTRL_VSYNC =
      0x00000002; // bit 1: vsync interrupt enable
  static constexpr u32 FRM_HWCTRL_HSYNC =
      0x00000004; // bit 2: hsync interrupt enable

  // FRM_CONTROL bitfields (0x3000C)
  static constexpr u32 FRM_CTRL_ENABLE =
      0x00000001; // bit 0: framebuffer enable
  static constexpr u32 FRM_CTRL_DOUBLE_BUF =
      0x00000002;                                   // bit 1: double buffering
  static constexpr u32 FRM_CTRL_SWAP = 0x00000004;  // bit 2: swap buffers
  static constexpr u32 FRM_CTRL_VSYNC = 0x00000008; // bit 3: vsync interrupt
  static constexpr u32 FRM_CTRL_HSYNC = 0x00000010; // bit 4: hsync interrupt

  // Getters for framebuffer widget
  u32 get_fb_base() const { return fb_base_; }
  u32 get_fb_stride() const { return fb_stride_; }
  u32 get_fb_width() const { return fb_width_; }
  u32 get_fb_height() const { return fb_height_; }
  u32 get_fb_depth() const { return fb_depth_; }
  bool is_linear() const { return linear_mode_; }
  u32 get_tile_ptr() const { return tile_ptr_; }

  // True once the framebuffer plane registers have been programmed
  // (i.e. the PROM/driver has actually configured a display surface).
  bool is_configured() const {
    return frm_size_tile_ != 0 || frm_size_pixel_ != 0 || frm_control_ != 0;
  }

private:
  // Register state
  u32 frm_size_tile_ = 0;
  u32 frm_size_pixel_ = 0;
  u32 frm_inhwctrl_ = 0;
  u32 frm_control_ = 0;

  // Derived framebuffer parameters (for framebuffer widget)
  u32 fb_base_ = 0;
  u32 fb_stride_ = 0;
  u32 fb_width_ = 0;
  u32 fb_height_ = 0;
  u32 fb_depth_ = 0;
  bool linear_mode_ = false;
  u32 tile_ptr_ = 0;

  void update_derived_params();
};

} // namespace o2emu::graphics