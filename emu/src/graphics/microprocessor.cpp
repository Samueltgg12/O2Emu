/**
 * @file microprocessor.cpp
 * @brief CRM Microprocessor (display list / vertex processing) implementation
 */

#include <cmath>
#include <cstring>
#include <o2emu/graphics/microprocessor.h>
#include <o2emu/logging/logger.h>

namespace o2emu::graphics {

Microprocessor::Microprocessor() { reset(); }

Microprocessor::~Microprocessor() = default;

u32 Microprocessor::read(Register reg) {
  switch (reg) {
  case MICRO_CMD_FIFO_STATUS:
    return (fifo_full() ? 0x2 : 0) | (fifo_empty() ? 0x1 : 0);

  case MICRO_STATUS:
    return status_;

  case MICRO_CONTROL:
    return control_;

  case MICRO_REVISION:
    return 0x00010000; // Version 1.0

  default:
    O2EMU_LOG_DEBUG("Microprocessor read from unknown register: 0x"
                    << std::hex << reg << std::dec);
    return 0;
  }
}

void Microprocessor::write(Register reg, u32 value) {
  switch (reg) {
  case MICRO_CMD_FIFO:
    push_command(value);
    break;

  case MICRO_CONTROL:
    control_ = value;
    enabled_ = (value & CTRL_ENABLE) != 0;
    if (value & CTRL_RESET) {
      reset();
    }
    break;

  case MICRO_RESET:
    if (value & 0x1) {
      reset();
    }
    break;

  case MICRO_VERTEX_X:
    vertex_[0] = *reinterpret_cast<const float *>(&value);
    break;
  case MICRO_VERTEX_Y:
    vertex_[1] = *reinterpret_cast<const float *>(&value);
    break;
  case MICRO_VERTEX_Z:
    vertex_[2] = *reinterpret_cast<const float *>(&value);
    break;
  case MICRO_VERTEX_W:
    vertex_[3] = *reinterpret_cast<const float *>(&value);
    break;
  case MICRO_VERTEX_R:
    vertex_[4] = *reinterpret_cast<const float *>(&value);
    break;
  case MICRO_VERTEX_G:
    vertex_[5] = *reinterpret_cast<const float *>(&value);
    break;
  case MICRO_VERTEX_B:
    vertex_[6] = *reinterpret_cast<const float *>(&value);
    break;
  case MICRO_VERTEX_A:
    vertex_[7] = *reinterpret_cast<const float *>(&value);
    break;
  case MICRO_VERTEX_S:
    vertex_[8] = *reinterpret_cast<const float *>(&value);
    break;
  case MICRO_VERTEX_T:
    vertex_[9] = *reinterpret_cast<const float *>(&value);
    break;

  case MICRO_MATRIX_MODE:
    matrix_mode_ = value & 0x3;
    break;

  case MICRO_MATRIX_LOAD:
    // Load matrix from command FIFO data
    break;

  case MICRO_MATRIX_MULT:
    // Multiply matrix from command FIFO data
    break;

  case MICRO_MATRIX_PUSH:
    push_matrix();
    break;

  case MICRO_MATRIX_POP:
    pop_matrix();
    break;

  case MICRO_LIGHT_ENABLE:
    if (value < 8) {
      lights_[value].enabled = true;
    }
    break;

  case MICRO_LIGHT_AMBIENT:
  case MICRO_LIGHT_DIFFUSE:
  case MICRO_LIGHT_SPECULAR:
  case MICRO_LIGHT_POSITION:
  case MICRO_LIGHT_DIRECTION:
  case MICRO_LIGHT_ATTENUATION:
    // Light parameters set via separate calls
    break;

  case MICRO_MATERIAL_AMBIENT:
  case MICRO_MATERIAL_DIFFUSE:
  case MICRO_MATERIAL_SPECULAR:
  case MICRO_MATERIAL_EMISSION:
  case MICRO_MATERIAL_SHININESS:
    // Material parameters set via separate calls
    break;

  case MICRO_VIEWPORT_X:
    viewport_x_ = static_cast<int>(value);
    break;
  case MICRO_VIEWPORT_Y:
    viewport_y_ = static_cast<int>(value);
    break;
  case MICRO_VIEWPORT_WIDTH:
    viewport_width_ = static_cast<int>(value);
    break;
  case MICRO_VIEWPORT_HEIGHT:
    viewport_height_ = static_cast<int>(value);
    break;
  case MICRO_VIEWPORT_MINZ:
    viewport_minz_ = *reinterpret_cast<const float *>(&value);
    break;
  case MICRO_VIEWPORT_MAXZ:
    viewport_maxz_ = *reinterpret_cast<const float *>(&value);
    break;

  case MICRO_CLIP_PLANE:
    // Clip plane set via separate calls
    break;

  default:
    O2EMU_LOG_DEBUG("Microprocessor write to unknown register: 0x"
                    << std::hex << reg << std::dec << " = 0x" << value);
    break;
  }
}

void Microprocessor::push_command(u32 cmd) {
  if (!fifo_full()) {
    cmd_fifo_[fifo_head_] = cmd;
    fifo_head_ = (fifo_head_ + 1) % 256;
    status_ &= ~STATUS_FIFO_EMPTY;
    if (fifo_full()) {
      status_ |= STATUS_FIFO_FULL;
    }
  }
}

bool Microprocessor::fifo_full() const {
  return ((fifo_head_ + 1) % 256) == fifo_tail_;
}

bool Microprocessor::fifo_empty() const { return fifo_head_ == fifo_tail_; }

void Microprocessor::process_vertex(float x, float y, float z, float w, float r,
                                    float g, float b, float a, float s,
                                    float t) {
  vertex_[0] = x;
  vertex_[1] = y;
  vertex_[2] = z;
  vertex_[3] = w;
  vertex_[4] = r;
  vertex_[5] = g;
  vertex_[6] = b;
  vertex_[7] = a;
  vertex_[8] = s;
  vertex_[9] = t;

  // Transform vertex by current matrix
  // Simplified - just pass through for now
  status_ |= STATUS_BUSY;
}

void Microprocessor::load_matrix(const float *matrix) {
  if (matrix_stack_depth_ < 32) {
    std::memcpy(matrix_stack_[matrix_stack_depth_], matrix, 16 * sizeof(float));
  }
}

void Microprocessor::multiply_matrix(const float *matrix) {
  // Matrix multiplication - simplified
  float result[16] = {};
  float *current = matrix_stack_[matrix_stack_depth_];

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      float sum = 0;
      for (int k = 0; k < 4; k++) {
        sum += current[i * 4 + k] * matrix[k * 4 + j];
      }
      result[i * 4 + j] = sum;
    }
  }

  std::memcpy(current, result, 16 * sizeof(float));
}

void Microprocessor::push_matrix() {
  if (matrix_stack_depth_ < 31) {
    std::memcpy(matrix_stack_[matrix_stack_depth_ + 1],
                matrix_stack_[matrix_stack_depth_], 16 * sizeof(float));
    matrix_stack_depth_++;
  }
}

void Microprocessor::pop_matrix() {
  if (matrix_stack_depth_ > 0) {
    matrix_stack_depth_--;
  }
}

void Microprocessor::set_light(int index, bool enable, const float *ambient,
                               const float *diffuse, const float *specular,
                               const float *position, const float *direction,
                               const float *attenuation) {
  if (index >= 0 && index < 8) {
    lights_[index].enabled = enable;
    if (ambient)
      std::memcpy(lights_[index].ambient, ambient, 4 * sizeof(float));
    if (diffuse)
      std::memcpy(lights_[index].diffuse, diffuse, 4 * sizeof(float));
    if (specular)
      std::memcpy(lights_[index].specular, specular, 4 * sizeof(float));
    if (position)
      std::memcpy(lights_[index].position, position, 4 * sizeof(float));
    if (direction)
      std::memcpy(lights_[index].direction, direction, 3 * sizeof(float));
    if (attenuation)
      std::memcpy(lights_[index].attenuation, attenuation, 3 * sizeof(float));
  }
}

void Microprocessor::set_material(const float *ambient, const float *diffuse,
                                  const float *specular, const float *emission,
                                  float shininess) {
  if (ambient)
    std::memcpy(material_ambient_, ambient, 4 * sizeof(float));
  if (diffuse)
    std::memcpy(material_diffuse_, diffuse, 4 * sizeof(float));
  if (specular)
    std::memcpy(material_specular_, specular, 4 * sizeof(float));
  if (emission)
    std::memcpy(material_emission_, emission, 4 * sizeof(float));
  material_shininess_ = shininess;
}

void Microprocessor::set_viewport(int x, int y, int width, int height,
                                  float minz, float maxz) {
  viewport_x_ = x;
  viewport_y_ = y;
  viewport_width_ = width;
  viewport_height_ = height;
  viewport_minz_ = minz;
  viewport_maxz_ = maxz;
}

void Microprocessor::set_clip_plane(int index, const float *plane) {
  if (index >= 0 && index < 6 && plane) {
    std::memcpy(clip_planes_[index], plane, 4 * sizeof(float));
  }
}

void Microprocessor::draw_primitive(Command cmd) {
  // Process command FIFO
  while (!fifo_empty()) {
    u32 cmd = cmd_fifo_[fifo_tail_];
    fifo_tail_ = (fifo_tail_ + 1) % 256;

    switch (cmd & 0xFF) {
    case CMD_NOP:
      break;
    case CMD_VERTEX:
      // Vertex already in vertex_ array
      break;
    case CMD_LINE:
    case CMD_TRIANGLE:
    case CMD_QUAD:
      // Rasterize primitive - would call MRE
      break;
    case CMD_CLEAR:
      // Clear buffers
      break;
    case CMD_FLUSH:
      flush();
      break;
    default:
      O2EMU_LOG_DEBUG("Microprocessor unknown command: 0x" << std::hex << cmd
                                                           << std::dec);
      break;
    }
  }

  status_ &= ~STATUS_FIFO_FULL;
  if (fifo_empty()) {
    status_ |= STATUS_FIFO_EMPTY;
  }
}

void Microprocessor::clear_buffers(u32 color, float depth) {
  // Clear color and depth buffers - would call MRE
  O2EMU_LOG_DEBUG("Microprocessor clear buffers: color=0x"
                  << std::hex << color << " depth=" << depth);
}

void Microprocessor::flush() {
  // Flush rendering pipeline
  status_ &= ~STATUS_BUSY;
  status_ |= STATUS_INTERRUPT;
}

bool Microprocessor::busy() const { return busy_; }

u32 Microprocessor::status() const { return status_; }

void Microprocessor::reset() {
  std::memset(regs_.data(), 0, regs_.size() * sizeof(u32));
  enabled_ = false;
  busy_ = false;
  status_ = STATUS_FIFO_EMPTY;
  control_ = 0;
  fifo_head_ = 0;
  fifo_tail_ = 0;
  std::memset(vertex_, 0, sizeof(vertex_));
  matrix_stack_depth_ = 0;
  matrix_mode_ = 0;
  std::memset(matrix_stack_, 0, sizeof(matrix_stack_));
  for (auto &light : lights_) {
    light = Light{};
  }
  std::memcpy(material_ambient_, (float[4]){0.2f, 0.2f, 0.2f, 1.0f},
              4 * sizeof(float));
  std::memcpy(material_diffuse_, (float[4]){0.8f, 0.8f, 0.8f, 1.0f},
              4 * sizeof(float));
  std::memcpy(material_specular_, (float[4]){0.0f, 0.0f, 0.0f, 1.0f},
              4 * sizeof(float));
  std::memcpy(material_emission_, (float[4]){0.0f, 0.0f, 0.0f, 1.0f},
              4 * sizeof(float));
  material_shininess_ = 0.0f;
  viewport_x_ = 0;
  viewport_y_ = 0;
  viewport_width_ = 640;
  viewport_height_ = 480;
  viewport_minz_ = 0.0f;
  viewport_maxz_ = 1.0f;
  std::memset(clip_planes_, 0, sizeof(clip_planes_));
}

} // namespace o2emu::graphics