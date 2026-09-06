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

void FramebufferWidget::setCPU(o2emu::cpu::ICpu *cpu) { cpu_ = cpu; }

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

  // Read framebuffer configuration from MRE/Display Engine
  // For now, use default values
  // TODO: Read from actual MRE registers

  makeCurrent();
  updateTexture();
  doneCurrent();

  frame_count_++;
  update(); // Trigger repaint
}

void FramebufferWidget::initializeGL() {
  initializeOpenGLFunctions();

  // Create shader program
  shader_program_ = this->glCreateProgram();

  GLuint vs = this->glCreateShader(GL_VERTEX_SHADER);
  this->glShaderSource(vs, 1, &vertex_shader_source, nullptr);
  this->glCompileShader(vs);

  GLuint fs = this->glCreateShader(GL_FRAGMENT_SHADER);
  this->glShaderSource(fs, 1, &fragment_shader_source, nullptr);
  this->glCompileShader(fs);

  this->glAttachShader(shader_program_, vs);
  this->glAttachShader(shader_program_, fs);
  this->glLinkProgram(shader_program_);

  this->glDeleteShader(vs);
  this->glDeleteShader(fs);

  // Check for errors
  GLint success;
  this->glGetProgramiv(shader_program_, GL_LINK_STATUS, &success);
  if (!success) {
    char log[512];
    this->glGetProgramInfoLog(shader_program_, 512, nullptr, log);
    qDebug() << "Shader link error:" << log;
  }

  // Create VAO and VBO for full-screen quad
  this->glGenVertexArrays(1, &vao_);
  this->glGenBuffers(1, &vbo_);

  this->glBindVertexArray(vao_);
  this->glBindBuffer(GL_ARRAY_BUFFER, vbo_);

  // Full-screen quad vertices (position, texCoord)
  float vertices[] = {
      // Positions        // TexCoords
      -1.0f, 1.0f,  0.0f, 0.0f, // Top-left
      -1.0f, -1.0f, 0.0f, 1.0f, // Bottom-left
      1.0f,  1.0f,  1.0f, 0.0f, // Top-right
      1.0f,  -1.0f, 1.0f, 1.0f, // Bottom-right
  };

  this->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                     GL_STATIC_DRAW);

  this->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                              (void *)0);
  this->glEnableVertexAttribArray(0);
  this->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                              (void *)(2 * sizeof(float)));
  this->glEnableVertexAttribArray(1);

  this->glBindVertexArray(0);

  // Create texture
  this->glGenTextures(1, &texture_id_);
  this->glBindTexture(GL_TEXTURE_2D, texture_id_);
  this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  this->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Initialize with black
  static std::vector<u8> black(1280 * 1024 * 4, 0);
  this->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1280, 1024, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, black.data());

  this->glBindTexture(GL_TEXTURE_2D, 0);

  this->glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
}

void FramebufferWidget::resizeGL(int w, int h) { this->glViewport(0, 0, w, h); }

void FramebufferWidget::paintGL() {
  this->glClear(GL_COLOR_BUFFER_BIT);

  if (shader_program_ && texture_id_) {
    this->glUseProgram(shader_program_);
    this->glBindVertexArray(vao_);
    this->glBindTexture(GL_TEXTURE_2D, texture_id_);
    this->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    this->glBindVertexArray(0);
    this->glUseProgram(0);
  }
}

void FramebufferWidget::updateTexture() {
  if (!memory_ || !texture_id_)
    return;

  // TODO: Read actual framebuffer from MRE/Display Engine registers
  // For now, create a test pattern

  const int width = 1280;
  const int height = 1024;
  const int bytes_per_pixel = 4;

  static std::vector<u8> pixels(width * height * bytes_per_pixel);

  // Generate test pattern (color bars)
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int idx = (y * width + x) * 4;
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

  this->glBindTexture(GL_TEXTURE_2D, texture_id_);
  this->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA,
                        GL_UNSIGNED_BYTE, pixels.data());
  this->glBindTexture(GL_TEXTURE_2D, 0);
}