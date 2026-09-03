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
    glDeleteTextures(1, &texture_id_);
  if (vao_)
    glDeleteVertexArrays(1, &vao_);
  if (vbo_)
    glDeleteBuffers(1, &vbo_);
  if (shader_program_)
    glDeleteProgram(shader_program_);
  doneCurrent();
}

void FramebufferWidget::setMemory(o2emu::memory::Memory *memory) {
  memory_ = memory;
}

void FramebufferWidget::setCPU(o2emu::cpu::ICpu *cpu) { cpu_ = cpu; }

void FramebufferWidget::clear() {
  makeCurrent();
  if (texture_id_) {
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    // Clear to black
    static std::vector<u8> black(1280 * 1024 * 4, 0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1280, 1024, GL_RGBA,
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
  static std::vector<u8> black(1280 * 1024 * 4, 0);
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

  glBindTexture(GL_TEXTURE_2D, texture_id_);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA,
                  GL_UNSIGNED_BYTE, pixels.data());
  glBindTexture(GL_TEXTURE_2D, 0);
}