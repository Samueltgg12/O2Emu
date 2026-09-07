/**
 * @file framebufferwidget.cpp
 * @brief Framebuffer widget implementation
 */

#include "framebufferwidget.h"
#include <QCursor>
#include <QDebug>
#include <QFocusEvent>
#include <QGuiApplication>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QPoint>
#include <cstring>
#include <o2emu/devices/ps2.h>
#include <o2emu/graphics/gbe_framebuffer.h>
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

void FramebufferWidget::setGBEFramebuffer(
    o2emu::graphics::GBEFramebuffer *framebuffer) {
  gbe_framebuffer_ = framebuffer;
}

void FramebufferWidget::setPS2(o2emu::devices::PS2 *ps2) { ps2_ = ps2; }

void FramebufferWidget::setCPU(o2emu::cpu::CPU *cpu) { cpu_ = cpu; }

void FramebufferWidget::grabInput() {
  if (input_grabbed_)
    return;

  input_grabbed_ = true;
  setFocus(Qt::MouseFocusReason);
  grabMouse();
  grabKeyboard();
  setCursor(Qt::BlankCursor);
  last_mouse_position_ = mapFromGlobal(QCursor::pos());
}

void FramebufferWidget::releaseInput() {
  if (!input_grabbed_)
    return;

  input_grabbed_ = false;
  releaseKeyboard();
  releaseMouse();
  unsetCursor();
}

static bool is_right_control(const QKeyEvent *event) {
  return event->key() == Qt::Key_Control &&
         (event->nativeScanCode() == 105 || event->nativeScanCode() == 0xE01D);
}

static uint32_t set2_scancode(int key) {
  switch (key) {
  case Qt::Key_A:
    return 0x1C;
  case Qt::Key_B:
    return 0x32;
  case Qt::Key_C:
    return 0x21;
  case Qt::Key_D:
    return 0x23;
  case Qt::Key_E:
    return 0x24;
  case Qt::Key_F:
    return 0x2B;
  case Qt::Key_G:
    return 0x34;
  case Qt::Key_H:
    return 0x33;
  case Qt::Key_I:
    return 0x43;
  case Qt::Key_J:
    return 0x3B;
  case Qt::Key_K:
    return 0x42;
  case Qt::Key_L:
    return 0x4B;
  case Qt::Key_M:
    return 0x3A;
  case Qt::Key_N:
    return 0x31;
  case Qt::Key_O:
    return 0x44;
  case Qt::Key_P:
    return 0x4D;
  case Qt::Key_Q:
    return 0x15;
  case Qt::Key_R:
    return 0x2D;
  case Qt::Key_S:
    return 0x1B;
  case Qt::Key_T:
    return 0x2C;
  case Qt::Key_U:
    return 0x3C;
  case Qt::Key_V:
    return 0x2A;
  case Qt::Key_W:
    return 0x1D;
  case Qt::Key_X:
    return 0x22;
  case Qt::Key_Y:
    return 0x35;
  case Qt::Key_Z:
    return 0x1A;
  case Qt::Key_0:
    return 0x45;
  case Qt::Key_1:
    return 0x16;
  case Qt::Key_2:
    return 0x1E;
  case Qt::Key_3:
    return 0x26;
  case Qt::Key_4:
    return 0x25;
  case Qt::Key_5:
    return 0x2E;
  case Qt::Key_6:
    return 0x36;
  case Qt::Key_7:
    return 0x3D;
  case Qt::Key_8:
    return 0x3E;
  case Qt::Key_9:
    return 0x46;
  case Qt::Key_Space:
    return 0x29;
  case Qt::Key_Return:
    return 0x5A;
  case Qt::Key_Escape:
    return 0x76;
  case Qt::Key_Backspace:
    return 0x66;
  case Qt::Key_Tab:
    return 0x0D;
  case Qt::Key_Left:
    return 0x6B;
  case Qt::Key_Right:
    return 0x74;
  case Qt::Key_Up:
    return 0x75;
  case Qt::Key_Down:
    return 0x72;
  case Qt::Key_Shift:
    return 0x12;
  case Qt::Key_Control:
    return 0x14;
  case Qt::Key_Alt:
    return 0x11;
  case Qt::Key_F1:
    return 0x05;
  case Qt::Key_F2:
    return 0x06;
  case Qt::Key_F3:
    return 0x04;
  case Qt::Key_F4:
    return 0x0C;
  case Qt::Key_F5:
    return 0x03;
  case Qt::Key_F6:
    return 0x0B;
  case Qt::Key_F7:
    return 0x83;
  case Qt::Key_F8:
    return 0x0A;
  case Qt::Key_F9:
    return 0x01;
  case Qt::Key_F10:
    return 0x09;
  case Qt::Key_F11:
    return 0x78;
  case Qt::Key_F12:
    return 0x07;
  default:
    return 0;
  }
}

void FramebufferWidget::sendKey(uint32_t key, bool pressed) {
  if (!ps2_ || key == 0)
    return;
  if (!pressed)
    ps2_->push_keyboard_scancode(0xF0);
  ps2_->push_keyboard_scancode(static_cast<o2emu::u8>(key));
}

void FramebufferWidget::mousePressEvent(QMouseEvent *event) {
  if (!input_grabbed_)
    grabInput();

  if (ps2_) {
    uint8_t buttons = 0;
    if (event->buttons() & Qt::LeftButton)
      buttons |= 1;
    if (event->buttons() & Qt::RightButton)
      buttons |= 2;
    if (event->buttons() & Qt::MiddleButton)
      buttons |= 4;
    ps2_->push_mouse_data(buttons, 0, 0);
  }
  event->accept();
}

void FramebufferWidget::mouseMoveEvent(QMouseEvent *event) {
  if (input_grabbed_ && ps2_) {
    const QPoint delta = event->position().toPoint() - last_mouse_position_;
    if (!delta.isNull()) {
      uint8_t buttons = 0;
      if (event->buttons() & Qt::LeftButton)
        buttons |= 1;
      if (event->buttons() & Qt::RightButton)
        buttons |= 2;
      if (event->buttons() & Qt::MiddleButton)
        buttons |= 4;
      ps2_->push_mouse_data(buttons, static_cast<o2emu::i8>(delta.x()),
                            static_cast<o2emu::i8>(-delta.y()));
    }
  }
  last_mouse_position_ = event->position().toPoint();
  event->accept();
}

void FramebufferWidget::keyPressEvent(QKeyEvent *event) {
  if (!input_grabbed_)
    return;
  if (is_right_control(event)) {
    releaseInput();
    event->accept();
    return;
  }
  if (!event->isAutoRepeat())
    sendKey(set2_scancode(event->key()), true);
  event->accept();
}

void FramebufferWidget::keyReleaseEvent(QKeyEvent *event) {
  if (input_grabbed_ && !is_right_control(event) && !event->isAutoRepeat())
    sendKey(set2_scancode(event->key()), false);
  event->accept();
}

void FramebufferWidget::focusOutEvent(QFocusEvent *event) {
  releaseInput();
  QOpenGLWidget::focusOutEvent(event);
}

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

  if (gbe_framebuffer_) {
    fb_base_ = gbe_framebuffer_->get_fb_base();
    fb_stride_ = gbe_framebuffer_->get_fb_stride();
    fb_width_ = gbe_framebuffer_->get_fb_width();
    fb_height_ = gbe_framebuffer_->get_fb_height();
    fb_depth_ = gbe_framebuffer_->get_fb_depth();

    // If the GBE framebuffer plane has never been programmed (PROM/driver
    // hasn't configured a display surface yet), leave fb_base_ at 0 so the
    // test-pattern fallback in updateTexture() is reachable. Only use the
    // real framebuffer when the device is actually configured.
    if (!gbe_framebuffer_->is_configured()) {
      fb_base_ = 0;
    }
  }

  if (fb_width_ == 0 || fb_height_ == 0 || fb_depth_ == 0) {
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