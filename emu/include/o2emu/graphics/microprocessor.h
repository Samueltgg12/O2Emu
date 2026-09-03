/**
 * @file microprocessor.h
 * @brief CRM Microprocessor (display list / vertex processing)
 *
 * Based on IRIX crm_micro.h and leaked IRIX source
 * The Microprocessor is the geometry engine of the CRM chipset
 */

#pragma once

#include <array>
#include <o2emu/o2emu.h>

namespace o2emu::graphics {

class Microprocessor {
public:
  Microprocessor();
  ~Microprocessor() = default;

  // Microprocessor register offsets (from PHYS_BASE_RENDER + offset)
  // Based on IRIX crm_micro.h and PROM definitions
  enum Register : uint32_t {
    // Command FIFO
    MICRO_CMD_FIFO = 0x000000,
    MICRO_CMD_FIFO_STATUS = 0x000004,

    // Vertex processing
    MICRO_VERTEX_BASE = 0x001000,
    MICRO_VERTEX_X = 0x001000,
    MICRO_VERTEX_Y = 0x001004,
    MICRO_VERTEX_Z = 0x001008,
    MICRO_VERTEX_W = 0x00100C,
    MICRO_VERTEX_R = 0x001010,
    MICRO_VERTEX_G = 0x001014,
    MICRO_VERTEX_B = 0x001018,
    MICRO_VERTEX_A = 0x00101C,
    MICRO_VERTEX_S = 0x001020,
    MICRO_VERTEX_T = 0x001024,

    // Matrix stack
    MICRO_MATRIX_BASE = 0x002000,
    MICRO_MATRIX_MODE = 0x002000,
    MICRO_MATRIX_LOAD = 0x002004,
    MICRO_MATRIX_MULT = 0x002008,
    MICRO_MATRIX_PUSH = 0x00200C,
    MICRO_MATRIX_POP = 0x002010,

    // Lighting
    MICRO_LIGHT_BASE = 0x003000,
    MICRO_LIGHT_ENABLE = 0x003000,
    MICRO_LIGHT_AMBIENT = 0x003004,
    MICRO_LIGHT_DIFFUSE = 0x003008,
    MICRO_LIGHT_SPECULAR = 0x00300C,
    MICRO_LIGHT_POSITION = 0x003010,
    MICRO_LIGHT_DIRECTION = 0x003014,
    MICRO_LIGHT_ATTENUATION = 0x003018,

    // Material
    MICRO_MATERIAL_BASE = 0x004000,
    MICRO_MATERIAL_AMBIENT = 0x004000,
    MICRO_MATERIAL_DIFFUSE = 0x004004,
    MICRO_MATERIAL_SPECULAR = 0x004008,
    MICRO_MATERIAL_EMISSION = 0x00400C,
    MICRO_MATERIAL_SHININESS = 0x004010,

    // Viewport/Clip
    MICRO_VIEWPORT_BASE = 0x005000,
    MICRO_VIEWPORT_X = 0x005000,
    MICRO_VIEWPORT_Y = 0x005004,
    MICRO_VIEWPORT_WIDTH = 0x005008,
    MICRO_VIEWPORT_HEIGHT = 0x00500C,
    MICRO_VIEWPORT_MINZ = 0x005010,
    MICRO_VIEWPORT_MAXZ = 0x005014,

    // Clip planes
    MICRO_CLIP_BASE = 0x006000,
    MICRO_CLIP_PLANE = 0x006000, // 6 clip planes

    // Status/Control
    MICRO_STATUS = 0x00FF00,
    MICRO_CONTROL = 0x00FF04,
    MICRO_RESET = 0x00FF08,
    MICRO_REVISION = 0x00FFFC,
  };

  // Command FIFO opcodes
  enum Command : uint32_t {
    CMD_NOP = 0x00,
    CMD_VERTEX = 0x01,
    CMD_LINE = 0x02,
    CMD_TRIANGLE = 0x03,
    CMD_QUAD = 0x04,
    CMD_MATRIX_LOAD = 0x10,
    CMD_MATRIX_MULT = 0x11,
    CMD_MATRIX_PUSH = 0x12,
    CMD_MATRIX_POP = 0x13,
    CMD_LIGHT_ENABLE = 0x20,
    CMD_LIGHT_DISABLE = 0x21,
    CMD_MATERIAL = 0x30,
    CMD_VIEWPORT = 0x40,
    CMD_CLIP_PLANE = 0x50,
    CMD_CLEAR = 0x60,
    CMD_FLUSH = 0x70,
  };

  // Status bits
  enum StatusBit : uint32_t {
    STATUS_BUSY = 0x00000001,
    STATUS_FIFO_FULL = 0x00000002,
    STATUS_FIFO_EMPTY = 0x00000004,
    STATUS_ERROR = 0x00000008,
    STATUS_INTERRUPT = 0x00000010,
  };

  // Control bits
  enum ControlBit : uint32_t {
    CTRL_ENABLE = 0x00000001,
    CTRL_RESET = 0x00000002,
    CTRL_INT_ENABLE = 0x00000004,
    CTRL_ZBUFFER = 0x00000010,
    CTRL_LIGHTING = 0x00000020,
    CTRL_TEXTURE = 0x00000040,
    CTRL_FOG = 0x00000080,
    CTRL_ALPHA_TEST = 0x00000100,
    CTRL_BLEND = 0x00000200,
    CTRL_DITHER = 0x00000400,
  };

  // Read/write registers
  u32 read(Register reg);
  void write(Register reg, u32 value);

  // Command FIFO
  void push_command(u32 cmd);
  bool fifo_full() const;
  bool fifo_empty() const;

  // Vertex processing
  void process_vertex(float x, float y, float z, float w, float r, float g,
                      float b, float a, float s, float t);

  // Matrix operations
  void load_matrix(const float *matrix);
  void multiply_matrix(const float *matrix);
  void push_matrix();
  void pop_matrix();

  // Lighting
  void set_light(int index, bool enable, const float *ambient,
                 const float *diffuse, const float *specular,
                 const float *position, const float *direction,
                 const float *attenuation);

  // Material
  void set_material(const float *ambient, const float *diffuse,
                    const float *specular, const float *emission,
                    float shininess);

  // Viewport
  void set_viewport(int x, int y, int width, int height, float minz,
                    float maxz);

  // Clip planes
  void set_clip_plane(int index, const float *plane);

  // Rendering
  void draw_primitive(Command cmd);
  void clear_buffers(u32 color, float depth);
  void flush();

  // Status
  bool busy() const;
  u32 status() const;

  // Reset
  void reset();

private:
  std::array<u32, 0x10000 / 4> regs_ = {}; // 64KB register space

  // State
  bool enabled_ = false;
  bool busy_ = false;
  u32 status_ = 0;
  u32 control_ = 0;

  // Command FIFO
  std::array<u32, 256> cmd_fifo_ = {};
  int fifo_head_ = 0;
  int fifo_tail_ = 0;

  // Vertex state
  float vertex_[10] = {}; // x, y, z, w, r, g, b, a, s, t

  // Matrix stack
  float matrix_stack_[32][16] = {};
  int matrix_stack_depth_ = 0;
  int matrix_mode_ = 0; // 0=modelview, 1=projection, 2=texture

  // Lighting
  struct Light {
    bool enabled = false;
    float ambient[4] = {0, 0, 0, 1};
    float diffuse[4] = {1, 1, 1, 1};
    float specular[4] = {1, 1, 1, 1};
    float position[4] = {0, 0, 1, 0};
    float direction[3] = {0, 0, -1};
    float attenuation[3] = {1, 0, 0};
  };
  std::array<Light, 8> lights_ = {};

  // Material
  float material_ambient_[4] = {0.2f, 0.2f, 0.2f, 1.0f};
  float material_diffuse_[4] = {0.8f, 0.8f, 0.8f, 1.0f};
  float material_specular_[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  float material_emission_[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  float material_shininess_ = 0.0f;

  // Viewport
  int viewport_x_ = 0;
  int viewport_y_ = 0;
  int viewport_width_ = 640;
  int viewport_height_ = 480;
  float viewport_minz_ = 0.0f;
  float viewport_maxz_ = 1.0f;

  // Clip planes
  float clip_planes_[6][4] = {};
};

} // namespace o2emu::graphics