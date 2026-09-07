/**
 * @file framebufferwidget.h
 * @brief Framebuffer display widget using OpenGL
 */

#pragma once

#include <QKeyEvent>
#include <QMouseEvent>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QTimer>
#include <cstdint>
#include <memory>

namespace o2emu::memory {
class Memory;
class MRE;
} // namespace o2emu::memory
namespace o2emu::graphics {
class GBEFramebuffer;
}
namespace o2emu::devices {
class PS2;
}
namespace o2emu::cpu {
class CPU;
}

class FramebufferWidget : public QOpenGLWidget,
                          protected QOpenGLFunctions_3_3_Core {
  Q_OBJECT

public:
  FramebufferWidget(QWidget *parent = nullptr);
  ~FramebufferWidget() override;

  void setMemory(o2emu::memory::Memory *memory);
  void setMRE(o2emu::memory::MRE *mre);
  void setGBEFramebuffer(o2emu::graphics::GBEFramebuffer *framebuffer);
  void setPS2(o2emu::devices::PS2 *ps2);
  void setCPU(o2emu::cpu::CPU *cpu);
  void clear();
  void updateFramebuffer();
  int fps() const { return fps_; }

protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;

private:
  o2emu::memory::Memory *memory_ = nullptr;
  o2emu::memory::MRE *mre_ = nullptr;
  o2emu::graphics::GBEFramebuffer *gbe_framebuffer_ = nullptr;
  o2emu::cpu::CPU *cpu_ = nullptr;
  o2emu::devices::PS2 *ps2_ = nullptr;
  bool input_grabbed_ = false;
  QPoint last_mouse_position_;

  // OpenGL resources
  GLuint texture_id_ = 0;
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  GLuint shader_program_ = 0;
  GLuint pbo_ = 0; // Pixel buffer object for async framebuffer upload
  GLsizei pbo_size_ = 0;

  // Framebuffer state
  uint32_t fb_base_ = 0;
  uint32_t fb_stride_ = 0;
  uint32_t fb_width_ = 1280;
  uint32_t fb_height_ = 1024;
  uint32_t fb_depth_ = 32;
  bool display_configured_ = false;
  bool tiled_display_ = false;

  // FPS tracking
  int fps_ = 0;
  int frame_count_ = 0;
  QTimer fps_timer_;

  // Shader sources
  static const char *vertex_shader_source;
  static const char *fragment_shader_source;

  void setupShaders();
  void updateTexture();
  void grabInput();
  void releaseInput();
  void sendKey(uint32_t key, bool pressed);
};