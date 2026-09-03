/**
 * @file framebufferwidget.h
 * @brief Framebuffer display widget using OpenGL
 */

#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QTimer>
#include <memory>

namespace o2emu::memory {
class Memory;
}
namespace o2emu::cpu {
class CPU;
}

class FramebufferWidget : public QOpenGLWidget, protected QOpenGLFunctions {
  Q_OBJECT

public:
  FramebufferWidget(QWidget *parent = nullptr);
  ~FramebufferWidget() override;

  void setMemory(o2emu::memory::Memory *memory);
  void setCPU(o2emu::cpu::CPU *cpu);
  void clear();
  void updateFramebuffer();
  int fps() const { return fps_; }

protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

private:
  o2emu::memory::Memory *memory_ = nullptr;
  o2emu::cpu::CPU *cpu_ = nullptr;

  // OpenGL resources
  GLuint texture_id_ = 0;
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  GLuint shader_program_ = 0;

  // Framebuffer state
  u32 fb_base_ = 0;
  u32 fb_stride_ = 0;
  u32 fb_width_ = 1280;
  u32 fb_height_ = 1024;
  u32 fb_depth_ = 32;

  // FPS tracking
  int fps_ = 0;
  int frame_count_ = 0;
  QTimer fps_timer_;

  // Shader sources
  static const char *vertex_shader_source;
  static const char *fragment_shader_source;

  void setupShaders();
  void updateTexture();
};