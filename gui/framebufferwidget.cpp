/**
 * @file framebufferwidget.cpp
 * @brief Framebuffer widget implementation
 */

#include "framebufferwidget.h"
#include <QDebug>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <cstring>
#include <o2emu/memory/memory.h>
#include <o2emu/o2emu.h>

const char *FramebufferWidget::vertex_shader_source = R"(
#version 330 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;
out vec2 vTexCoord;
void main() {
    vTexCoord = texCoord;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

const char *FramebufferWidget::fragment_shader_source = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D framebuffer;
void main() {
    fragColor = texture(framebuffer, vTexCoord);
}
)";

FramebufferWidget::FramebufferWidget(QWidget *parent) : QOpenGLWidget(parent) {
  setMinimumSize(640, 480);
  setFocusPolicy(Qt::StrongFocus);

  // FPS timer
  connect(&fps_timer_, &QTimer::timeout, [this]() {
    fps_ = frame_count_;
    frame_count_ = 0;
  });
  fps_timer_.start(1000);
}

FramebufferWidget::~FramebufferWidget() {
  makeCurrent();
  initializeOpenGLFunctions();
  if (texture_id_)
    this->glDeleteTextures(1, &texture_id_);
  if (vao_)
    this->glDeleteVertexArrays(1, &vao_);
  if (vbo_)
    this->glDeleteBuffers(1, &vbo_);
  if (shader_program_)
    this->glDeleteProgram(shader_program_);
  doneCurrent();
}

void FramebufferWidget::setMemory(o2emu::memory::Memory *memory) {
  memory_ = memory;
}

void FramebufferWidget::setCPU(o2emu::cpu::CPU *cpu) { cpu_ = cpu; }

void FramebufferWidget::clear() {
  makeCurrent();
  if (texture_id_) {
    this->glBindTexture(GL_TEXTURE_2D, texture_id_);
    // Clear to black
    static std::vector<o2emu::u8> black(1280 * 1024 * 4, 0);
    this->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1280, 1024, GL_RGBA,
                          GL_UNSIGNED_BYTE, black.data());
  }
  doneCurrent();
  update();
}

void FramebufferWidget::updateFramebuffer() {
  if (!memory_)
    return;

  // Read framebuffer configuration from GBE (Graphics Back End / Display
  // Engine) GBE base address: 0x16000000 Framebuffer plane registers at offset
  // 0x30000
  constexpr uint32_t GBE_BASE = 0x16000000;
  constexpr uint32_t FB_PLANE_OFFSET = 0x30000;

  // Read frm_size_tile register (offset 0x30000)
  // Bits 4:0   = FRM_RHS (right hand side / tile count - 1)
  // Bits 12:5  = FRM_WIDTH_TILE (width in tiles)
  // Bits 14:13 = FRM_DEPTH (0=8bpp, 1=16bpp, 2=24bpp, 3=32bpp)
  // Bit 15     = FRM_FIFO_RESET
  uint32_t frm_size_tile = memory_->read32(GBE_BASE + FB_PLANE_OFFSET + 0x00);

  // Read frm_size_pixel register (offset 0x30004)
  // Bits 31:16 = FB_HEIGHT_PIX (height in pixels)
  uint32_t frm_size_pixel = memory_->read32(GBE_BASE + FB_PLANE_OFFSET + 0x04);

  // Read frm_control register (offset 0x3000C)
  // Bit 0      = FRM_DMA_ENABLE
  // Bit 1      = FRM_LINEAR (1 = linear framebuffer, 0 = tiled)
  // Bits 31:9  = FRM_TILE_PTR (tile list pointer / framebuffer base)
  uint32_t frm_control = memory_->read32(GBE_BASE + FB_PLANE_OFFSET + 0x0C);

  // Parse framebuffer configuration
  uint32_t width_tiles =
      (frm_size_tile >> 5) & 0xFF;              // FRM_WIDTH_TILE (bits 12:5)
  uint32_t depth = (frm_size_tile >> 13) & 0x3; // FRM_DEPTH (bits 14:13)
  uint32_t height_pixels =
      (frm_size_pixel >> 16) & 0xFFFF;    // FB_HEIGHT_PIX (bits 31:16)
  bool linear = (frm_control >> 1) & 0x1; // FRM_LINEAR (bit 1)
  uint32_t tile_ptr = (frm_control >> 9) & 0x7FFFFF; // FRM_TILE_PTR (bits 31:9)

  // Calculate actual dimensions
  // Each tile is 128 bytes (32 pixels at 32bpp, 64 pixels at 16bpp, etc.)
  // Width in pixels = width_tiles * tiles_per_pixel_factor
  // For simplicity, assume 32 pixels per tile at 32bpp
  uint32_t pixels_per_tile = 32; // 128 bytes / 4 bytes per pixel at 32bpp
  uint32_t width = width_tiles * pixels_per_tile;
  uint32_t height = height_pixels;

  // Determine bytes per pixel from depth
  uint32_t bytes_per_pixel = 4; // default 32bpp
  switch (depth) {
  case 0:
    bytes_per_pixel = 1;
    break; // 8bpp
  case 1:
    bytes_per_pixel = 2;
    break; // 16bpp
  case 2:
    bytes_per_pixel = 3;
    break; // 24bpp
  case 3:
    bytes_per_pixel = 4;
    break; // 32bpp
  }

  // Calculate framebuffer base address from tile pointer
  // FRM_TILE_PTR is bits 31:9, so shift left by 9 to get byte address
  uint32_t fb_base = tile_ptr << 9;

  // If linear mode, the tile pointer may point directly to framebuffer
  // Otherwise, we'd need to follow the tile list (more complex)
  if (linear && fb_base != 0) {
    fb_base_ = fb_base;
    fb_width_ = width ? width : 1024;
    fb_height_ = height ? height : 768;
    fb_depth_ = bytes_per_pixel * 8;
    fb_stride_ = fb_width_ * bytes_per_pixel;
  } else if (fb_base_ == 0) {
    // Fallback to defaults if registers not configured
    fb_base_ = 0x00800000; // Typical framebuffer location in UMA
    fb_width_ = 1024;
    fb_height_ = 768;
    fb_depth_ = 32;
    fb_stride_ = fb_width_ * 4;
  }

  makeCurrent();
  updateTexture();
  doneCurrent();

  frame_count_++;
  update(); // Trigger repaint
}

void FramebufferWidget::initializeGL() {
  initializeOpenGLFunctions();

  // Create shader program
  shader_program_ = glCreateProgram();

  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vertex_shader_source, nullptr);
  glCompileShader(vs);

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fragment_shader_source, nullptr);
  glCompileShader(fs);

  glAttachShader(shader_program_, vs);
  glAttachShader(shader_program_, fs);
  glLinkProgram(shader_program_);

  glDeleteShader(vs);
  glDeleteShader(fs);

  // Check for errors
  GLint success;
  glGetProgramiv(shader_program_, GL_LINK_STATUS, &success);
  if (!success) {
    char log[512];
    glGetProgramInfoLog(shader_program_, 512, nullptr, log);
    qDebug() << "Shader link error:" << log;
  }

  // Create VAO and VBO for full-screen quad
  glGenVertexArrays(1, &vao_);
  glGenBuffers(1, &vbo_);

  glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);

  // Full-screen quad vertices (position, texCoord)
  float vertices[] = {
      // Positions        // TexCoords
      -1.0f, 1.0f,  0.0f, 0.0f, // Top-left
      -1.0f, -1.0f, 0.0f, 1.0f, // Bottom-left
      1.0f,  1.0f,  1.0f, 0.0f, // Top-right
      1.0f,  -1.0f, 1.0f, 1.0f, // Bottom-right
  };

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);

  // Create texture
  glGenTextures(1, &texture_id_);
  glBindTexture(GL_TEXTURE_2D, texture_id_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Initialize with black
  static std::vector<o2emu::u8> black(1280 * 1024 * 4, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1280, 1024, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, black.data());

  glBindTexture(GL_TEXTURE_2D, 0);

  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
}

void FramebufferWidget::resizeGL(int w, int h) { glViewport(0, 0, w, h); }

void FramebufferWidget::paintGL() {
  glClear(GL_COLOR_BUFFER_BIT);

  if (shader_program_ && texture_id_) {
    glUseProgram(shader_program_);
    glBindVertexArray(vao_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);
  }
}

void FramebufferWidget::updateTexture() {
  if (!memory_ || !texture_id_)
    return;

  // Use framebuffer configuration from registers
  uint32_t width = fb_width_;
  uint32_t height = fb_height_;
  uint32_t bytes_per_pixel = fb_depth_ / 8;
  uint32_t stride = fb_stride_;

  // Validate dimensions
  if (width == 0 || height == 0 || width > 4096 || height > 4096) {
    width = 1024;
    height = 768;
    bytes_per_pixel = 4;
    stride = width * bytes_per_pixel;
  }

  // Allocate buffer for framebuffer data
  std::vector<o2emu::u8> pixels(width * height *
                                4); // Always convert to RGBA for display

  // Read framebuffer data from memory
  if (fb_base_ != 0) {
    // Read row by row to handle stride correctly
    for (uint32_t y = 0; y < height; ++y) {
      uint32_t src_addr = fb_base_ + y * stride;
      uint32_t dst_offset = y * width * 4;

      // Read a row of pixels
      std::vector<o2emu::u8> row_data(stride);
      memory_->read_block(src_addr, row_data.data(), stride);

      // Convert to RGBA based on depth
      for (uint32_t x = 0; x < width; ++x) {
        uint32_t src_idx = x * bytes_per_pixel;
        uint32_t dst_idx = dst_offset + x * 4;

        if (src_idx + bytes_per_pixel <= stride) {
          switch (bytes_per_pixel) {
          case 1: { // 8bpp - indexed color, treat as grayscale
            o2emu::u8 val = row_data[src_idx];
            pixels[dst_idx + 0] = val;
            pixels[dst_idx + 1] = val;
            pixels[dst_idx + 2] = val;
            pixels[dst_idx + 3] = 255;
            break;
          }
          case 2: { // 16bpp - RGB565 or ARGB1555
            uint16_t pixel = *reinterpret_cast<uint16_t *>(&row_data[src_idx]);
            // Assume RGB565: 5 bits red, 6 bits green, 5 bits blue
            pixels[dst_idx + 0] = ((pixel >> 11) & 0x1F) << 3; // Red
            pixels[dst_idx + 1] = ((pixel >> 5) & 0x3F) << 2;  // Green
            pixels[dst_idx + 2] = (pixel & 0x1F) << 3;         // Blue
            pixels[dst_idx + 3] = 255;
            break;
          }
          case 3: {                                      // 24bpp - RGB888
            pixels[dst_idx + 0] = row_data[src_idx + 0]; // Red
            pixels[dst_idx + 1] = row_data[src_idx + 1]; // Green
            pixels[dst_idx + 2] = row_data[src_idx + 2]; // Blue
            pixels[dst_idx + 3] = 255;
            break;
          }
          case 4: { // 32bpp - ARGB8888 or RGBA8888
            pixels[dst_idx + 0] = row_data[src_idx + 0]; // Red
            pixels[dst_idx + 1] = row_data[src_idx + 1]; // Green
            pixels[dst_idx + 2] = row_data[src_idx + 2]; // Blue
            pixels[dst_idx + 3] = row_data[src_idx + 3]; // Alpha
            break;
          }
          }
        }
      }
    }
  } else {
    // Fallback: generate test pattern if no framebuffer configured
    for (uint32_t y = 0; y < height; ++y) {
      for (uint32_t x = 0; x < width; ++x) {
        uint32_t idx = (y * width + x) * 4;
        int bar = (x * 8) / width;
        switch (bar) {
        case 0:
          pixels[idx + 0] = 255;
          pixels[idx + 1] = 0;
          pixels[idx + 2] = 0;
          break; // Red
        case 1:
          pixels[idx + 0] = 255;
          pixels[idx + 1] = 255;
          pixels[idx + 2] = 0;
          break; // Yellow
        case 2:
          pixels[idx + 0] = 0;
          pixels[idx + 1] = 255;
          pixels[idx + 2] = 0;
          break; // Green
        case 3:
          pixels[idx + 0] = 0;
          pixels[idx + 1] = 255;
          pixels[idx + 2] = 255;
          break; // Cyan
        case 4:
          pixels[idx + 0] = 0;
          pixels[idx + 1] = 0;
          pixels[idx + 2] = 255;
          break; // Blue
        case 5:
          pixels[idx + 0] = 255;
          pixels[idx + 1] = 0;
          pixels[idx + 2] = 255;
          break; // Magenta
        case 6:
          pixels[idx + 0] = 255;
          pixels[idx + 1] = 255;
          pixels[idx + 2] = 255;
          break; // White
        case 7:
          pixels[idx + 0] = 0;
          pixels[idx + 1] = 0;
          pixels[idx + 2] = 0;
          break; // Black
        }
        pixels[idx + 3] = 255; // Alpha
      }
    }
  }

  // Update OpenGL texture
  // Recreate texture if dimensions changed
  glBindTexture(GL_TEXTURE_2D, texture_id_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, pixels.data());
  glBindTexture(GL_TEXTURE_2D, 0);
}