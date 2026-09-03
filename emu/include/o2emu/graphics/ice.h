/**
 * @file ice.h
 * @brief ICE (Imaging & Compression Engine) - part of CRM chipset
 *
 * Based on VICE Design Spec (docs/manuals-specs/o2-VICE-spec.md)
 * and IRIX crm_ice.h
 * ICE handles image compression/decompression (JPEG, MPEG, etc.)
 */

#pragma once

#include <array>
#include <o2emu/o2emu.h>

namespace o2emu::graphics {

class ICE {
public:
  ICE();
  ~ICE() = default;

  // ICE register offsets (from PHYS_BASE_RENDER + offset)
  // Based on VICE Design Spec and PROM definitions
  enum Register : uint32_t {
    // Control/Status
    ICE_CONTROL = 0x000000,
    ICE_STATUS = 0x000004,
    ICE_INTERRUPT = 0x000008,
    ICE_INTERRUPT_MASK = 0x00000C,

    // Command FIFO
    ICE_CMD_FIFO = 0x001000,
    ICE_CMD_FIFO_STATUS = 0x001004,

    // Source/Destination buffers
    ICE_SRC_BASE = 0x002000,
    ICE_SRC_ADDR = 0x002000,
    ICE_SRC_STRIDE = 0x002004,
    ICE_SRC_FORMAT = 0x002008,
    ICE_SRC_WIDTH = 0x00200C,
    ICE_SRC_HEIGHT = 0x002010,

    ICE_DST_BASE = 0x003000,
    ICE_DST_ADDR = 0x003000,
    ICE_DST_STRIDE = 0x003004,
    ICE_DST_FORMAT = 0x003008,
    ICE_DST_WIDTH = 0x00300C,
    ICE_DST_HEIGHT = 0x003010,

    // JPEG compression/decompression
    ICE_JPEG_BASE = 0x004000,
    ICE_JPEG_CONTROL = 0x004000,
    ICE_JPEG_QTABLE = 0x004100,    // 64 entries
    ICE_JPEG_HTABLE_DC = 0x004200, // Huffman tables
    ICE_JPEG_HTABLE_AC = 0x004300,
    ICE_JPEG_RESTART = 0x004400,

    // MPEG
    ICE_MPEG_BASE = 0x005000,
    ICE_MPEG_CONTROL = 0x005000,
    ICE_MPEG_QUANT = 0x005100,

    // Color space conversion
    ICE_CSC_BASE = 0x006000,
    ICE_CSC_MATRIX = 0x006000, // 3x3 matrix + offset
    ICE_CSC_OFFSET = 0x006024,

    // Scaling/Filtering
    ICE_SCALE_BASE = 0x007000,
    ICE_SCALE_H_FACTOR = 0x007000,
    ICE_SCALE_V_FACTOR = 0x007004,
    ICE_SCALE_FILTER = 0x007008,

    // Status/Control
    ICE_RESET = 0x00FF00,
    ICE_REVISION = 0x00FFFC,
  };

  // Control bits
  enum ControlBit : uint32_t {
    CTRL_ENABLE = 0x00000001,
    CTRL_RESET = 0x00000002,
    CTRL_INT_ENABLE = 0x00000004,
    CTRL_MODE_JPEG_ENC = 0x00000010,
    CTRL_MODE_JPEG_DEC = 0x00000020,
    CTRL_MODE_MPEG_ENC = 0x00000040,
    CTRL_MODE_MPEG_DEC = 0x00000080,
    CTRL_MODE_CSC = 0x00000100,
    CTRL_MODE_SCALE = 0x00000200,
    CTRL_DMA_ENABLE = 0x00001000,
  };

  // Status bits
  enum StatusBit : uint32_t {
    STATUS_BUSY = 0x00000001,
    STATUS_CMD_FIFO_FULL = 0x00000002,
    STATUS_CMD_FIFO_EMPTY = 0x00000004,
    STATUS_DATA_FIFO_FULL = 0x00000008,
    STATUS_DATA_FIFO_EMPTY = 0x00000010,
    STATUS_ERROR = 0x00000020,
    STATUS_INTERRUPT = 0x00000040,
    STATUS_JPEG_DONE = 0x00000100,
    STATUS_MPEG_DONE = 0x00000200,
    STATUS_CSC_DONE = 0x00000400,
    STATUS_SCALE_DONE = 0x00000800,
  };

  // Interrupt bits
  enum InterruptBit : uint32_t {
    INTR_JPEG_DONE = 0x00000001,
    INTR_MPEG_DONE = 0x00000002,
    INTR_CSC_DONE = 0x00000004,
    INTR_SCALE_DONE = 0x00000008,
    INTR_ERROR = 0x00000010,
    INTR_CMD_FIFO_EMPTY = 0x00000020,
    INTR_DATA_FIFO_FULL = 0x00000040,
  };

  // Source/Destination formats
  enum Format : uint32_t {
    FMT_RGB565 = 0x00,
    FMT_RGB888 = 0x01,
    FMT_RGBA8888 = 0x02,
    FMT_YUV422 = 0x10,
    FMT_YUV420 = 0x11,
    FMT_YUV411 = 0x12,
    FMT_JPEG = 0x20,
    FMT_MPEG = 0x30,
  };

  // Read/write registers
  u32 read(Register reg);
  void write(Register reg, u32 value);

  // Command FIFO
  void push_command(u32 cmd);
  bool fifo_full() const;
  bool fifo_empty() const;

  // JPEG operations
  void jpeg_compress(u32 src_addr, u32 dst_addr, u32 width, u32 height,
                     u32 quality);
  void jpeg_decompress(u32 src_addr, u32 dst_addr, u32 *width, u32 *height);

  // MPEG operations
  void mpeg_compress(u32 src_addr, u32 dst_addr, u32 width, u32 height,
                     u32 bitrate);
  void mpeg_decompress(u32 src_addr, u32 dst_addr, u32 *width, u32 *height);

  // Color space conversion
  void csc_convert(u32 src_addr, u32 dst_addr, u32 width, u32 height,
                   Format src_fmt, Format dst_fmt);

  // Scaling
  void scale_image(u32 src_addr, u32 dst_addr, u32 src_w, u32 src_h, u32 dst_w,
                   u32 dst_h, Format fmt);

  // Quantization tables
  void set_jpeg_qtable(const u8 *qtable);
  void set_jpeg_htable_dc(const u8 *htable);
  void set_jpeg_htable_ac(const u8 *htable);

  // CSC matrix
  void set_csc_matrix(const float *matrix, const float *offset);

  // Status
  bool busy() const;
  u32 status() const;
  u32 interrupt_status() const;

  // Reset
  void reset();

private:
  std::array<u32, 0x10000 / 4> regs_ = {}; // 64KB register space

  // State
  bool enabled_ = false;
  bool busy_ = false;
  u32 status_ = 0;
  u32 control_ = 0;
  u32 interrupt_mask_ = 0;
  u32 interrupt_status_ = 0;

  // Command FIFO
  std::array<u32, 256> cmd_fifo_ = {};
  int fifo_head_ = 0;
  int fifo_tail_ = 0;

  // JPEG state
  std::array<u8, 64> jpeg_qtable_ = {};
  std::array<u8, 256> jpeg_htable_dc_ = {};
  std::array<u8, 256> jpeg_htable_ac_ = {};

  // CSC matrix
  float csc_matrix_[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
  float csc_offset_[3] = {0, 0, 0};

  // Current operation
  enum Operation {
    OP_NONE,
    OP_JPEG_ENC,
    OP_JPEG_DEC,
    OP_MPEG_ENC,
    OP_MPEG_DEC,
    OP_CSC,
    OP_SCALE,
  } current_op_ = OP_NONE;
};

} // namespace o2emu::graphics