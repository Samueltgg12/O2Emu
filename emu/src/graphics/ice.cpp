/**
 * @file ice.cpp
 * @brief ICE (Imaging & Compression Engine) implementation
 */

#include <cstring>
#include <o2emu/graphics/ice.h>
#include <o2emu/logging/logger.h>

namespace o2emu::graphics {

ICE::ICE() { reset(); }

ICE::~ICE() = default;

u32 ICE::read(Register reg) {
  switch (reg) {
  case ICE_CONTROL:
    return control_;

  case ICE_STATUS:
    return status_;

  case ICE_INTERRUPT:
    return interrupt_status_;

  case ICE_INTERRUPT_MASK:
    return interrupt_mask_;

  case ICE_CMD_FIFO_STATUS:
    return (fifo_full() ? 0x2 : 0) | (fifo_empty() ? 0x1 : 0);

  case ICE_REVISION:
    return 0x00010000; // Version 1.0

  case ICE_SRC_ADDR:
    return regs_[ICE_SRC_ADDR];
  case ICE_SRC_STRIDE:
    return regs_[ICE_SRC_STRIDE];
  case ICE_SRC_FORMAT:
    return regs_[ICE_SRC_FORMAT];
  case ICE_SRC_WIDTH:
    return regs_[ICE_SRC_WIDTH];
  case ICE_SRC_HEIGHT:
    return regs_[ICE_SRC_HEIGHT];

  case ICE_DST_ADDR:
    return regs_[ICE_DST_ADDR];
  case ICE_DST_STRIDE:
    return regs_[ICE_DST_STRIDE];
  case ICE_DST_FORMAT:
    return regs_[ICE_DST_FORMAT];
  case ICE_DST_WIDTH:
    return regs_[ICE_DST_WIDTH];
  case ICE_DST_HEIGHT:
    return regs_[ICE_DST_HEIGHT];

  case ICE_JPEG_CONTROL:
    return regs_[ICE_JPEG_CONTROL];

  case ICE_CSC_MATRIX:
  case ICE_CSC_OFFSET:
    // Return CSC matrix/offset
    return 0;

  case ICE_SCALE_H_FACTOR:
    return regs_[ICE_SCALE_H_FACTOR];
  case ICE_SCALE_V_FACTOR:
    return regs_[ICE_SCALE_V_FACTOR];
  case ICE_SCALE_FILTER:
    return regs_[ICE_SCALE_FILTER];

  default:
    O2EMU_LOG_DEBUG("ICE read from unknown register: 0x" << std::hex << reg
                                                         << std::dec);
    return regs_[reg];
  }
}

void ICE::write(Register reg, u32 value) {
  switch (reg) {
  case ICE_CONTROL:
    control_ = value;
    enabled_ = (value & CTRL_ENABLE) != 0;
    if (value & CTRL_RESET) {
      reset();
    }
    break;

  case ICE_INTERRUPT_MASK:
    interrupt_mask_ = value;
    break;

  case ICE_CMD_FIFO:
    push_command(value);
    break;

  case ICE_SRC_ADDR:
    regs_[ICE_SRC_ADDR] = value;
    break;
  case ICE_SRC_STRIDE:
    regs_[ICE_SRC_STRIDE] = value;
    break;
  case ICE_SRC_FORMAT:
    regs_[ICE_SRC_FORMAT] = value;
    break;
  case ICE_SRC_WIDTH:
    regs_[ICE_SRC_WIDTH] = value;
    break;
  case ICE_SRC_HEIGHT:
    regs_[ICE_SRC_HEIGHT] = value;
    break;

  case ICE_DST_ADDR:
    regs_[ICE_DST_ADDR] = value;
    break;
  case ICE_DST_STRIDE:
    regs_[ICE_DST_STRIDE] = value;
    break;
  case ICE_DST_FORMAT:
    regs_[ICE_DST_FORMAT] = value;
    break;
  case ICE_DST_WIDTH:
    regs_[ICE_DST_WIDTH] = value;
    break;
  case ICE_DST_HEIGHT:
    regs_[ICE_DST_HEIGHT] = value;
    break;

  case ICE_JPEG_CONTROL:
    regs_[ICE_JPEG_CONTROL] = value;
    break;

  case ICE_JPEG_QTABLE:
    // Quantization table - 64 entries
    break;

  case ICE_JPEG_HTABLE_DC:
    // Huffman DC table
    break;

  case ICE_JPEG_HTABLE_AC:
    // Huffman AC table
    break;

  case ICE_JPEG_RESTART:
    regs_[ICE_JPEG_RESTART] = value;
    break;

  case ICE_MPEG_CONTROL:
    regs_[ICE_MPEG_CONTROL] = value;
    break;

  case ICE_MPEG_QUANT:
    // MPEG quantization table
    break;

  case ICE_CSC_MATRIX:
    // CSC matrix - 9 floats
    break;

  case ICE_CSC_OFFSET:
    // CSC offset - 3 floats
    break;

  case ICE_SCALE_H_FACTOR:
    regs_[ICE_SCALE_H_FACTOR] = value;
    break;
  case ICE_SCALE_V_FACTOR:
    regs_[ICE_SCALE_V_FACTOR] = value;
    break;
  case ICE_SCALE_FILTER:
    regs_[ICE_SCALE_FILTER] = value;
    break;

  case ICE_RESET:
    if (value & 0x1) {
      reset();
    }
    break;

  default:
    O2EMU_LOG_DEBUG("ICE write to unknown register: 0x"
                    << std::hex << reg << std::dec << " = 0x" << value);
    regs_[reg] = value;
    break;
  }
}

void ICE::push_command(u32 cmd) {
  if (!fifo_full()) {
    cmd_fifo_[fifo_head_] = cmd;
    fifo_head_ = (fifo_head_ + 1) % 256;
    status_ &= ~STATUS_CMD_FIFO_EMPTY;
    if (fifo_full()) {
      status_ |= STATUS_CMD_FIFO_FULL;
    }
  }
}

bool ICE::fifo_full() const { return ((fifo_head_ + 1) % 256) == fifo_tail_; }

bool ICE::fifo_empty() const { return fifo_head_ == fifo_tail_; }

void ICE::jpeg_compress(u32 src_addr, u32 dst_addr, u32 width, u32 height,
                        u32 quality) {
  if (!enabled_)
    return;

  current_op_ = OP_JPEG_ENC;
  busy_ = true;
  status_ |= STATUS_BUSY;

  // Set source/destination
  regs_[ICE_SRC_ADDR] = src_addr;
  regs_[ICE_DST_ADDR] = dst_addr;
  regs_[ICE_SRC_WIDTH] = width;
  regs_[ICE_SRC_HEIGHT] = height;
  regs_[ICE_DST_WIDTH] = width;
  regs_[ICE_DST_HEIGHT] = height;

  // Simulate compression
  O2EMU_LOG_DEBUG("ICE JPEG compress: " << width << "x" << height
                                        << " quality=" << quality);

  // In a real implementation, this would do actual JPEG compression
  // For now, just mark as done
  status_ &= ~STATUS_BUSY;
  status_ |= STATUS_JPEG_DONE;
  interrupt_status_ |= INTR_JPEG_DONE;
  busy_ = false;
}

void ICE::jpeg_decompress(u32 src_addr, u32 dst_addr, u32 *width, u32 *height) {
  if (!enabled_)
    return;

  current_op_ = OP_JPEG_DEC;
  busy_ = true;
  status_ |= STATUS_BUSY;

  regs_[ICE_SRC_ADDR] = src_addr;
  regs_[ICE_DST_ADDR] = dst_addr;

  // Simulate decompression
  O2EMU_LOG_DEBUG("ICE JPEG decompress");

  if (width)
    *width = 640;
  if (height)
    *height = 480;

  status_ &= ~STATUS_BUSY;
  status_ |= STATUS_JPEG_DONE;
  interrupt_status_ |= INTR_JPEG_DONE;
  busy_ = false;
}

void ICE::mpeg_compress(u32 src_addr, u32 dst_addr, u32 width, u32 height,
                        u32 bitrate) {
  if (!enabled_)
    return;

  current_op_ = OP_MPEG_ENC;
  busy_ = true;
  status_ |= STATUS_BUSY;

  regs_[ICE_SRC_ADDR] = src_addr;
  regs_[ICE_DST_ADDR] = dst_addr;
  regs_[ICE_SRC_WIDTH] = width;
  regs_[ICE_SRC_HEIGHT] = height;

  O2EMU_LOG_DEBUG("ICE MPEG compress: " << width << "x" << height
                                        << " bitrate=" << bitrate);

  status_ &= ~STATUS_BUSY;
  status_ |= STATUS_MPEG_DONE;
  interrupt_status_ |= INTR_MPEG_DONE;
  busy_ = false;
}

void ICE::mpeg_decompress(u32 src_addr, u32 dst_addr, u32 *width, u32 *height) {
  if (!enabled_)
    return;

  current_op_ = OP_MPEG_DEC;
  busy_ = true;
  status_ |= STATUS_BUSY;

  regs_[ICE_SRC_ADDR] = src_addr;
  regs_[ICE_DST_ADDR] = dst_addr;

  O2EMU_LOG_DEBUG("ICE MPEG decompress");

  if (width)
    *width = 640;
  if (height)
    *height = 480;

  status_ &= ~STATUS_BUSY;
  status_ |= STATUS_MPEG_DONE;
  interrupt_status_ |= INTR_MPEG_DONE;
  busy_ = false;
}

void ICE::csc_convert(u32 src_addr, u32 dst_addr, u32 width, u32 height,
                      Format src_fmt, Format dst_fmt) {
  if (!enabled_)
    return;

  current_op_ = OP_CSC;
  busy_ = true;
  status_ |= STATUS_BUSY;

  regs_[ICE_SRC_ADDR] = src_addr;
  regs_[ICE_DST_ADDR] = dst_addr;
  regs_[ICE_SRC_WIDTH] = width;
  regs_[ICE_SRC_HEIGHT] = height;
  regs_[ICE_SRC_FORMAT] = src_fmt;
  regs_[ICE_DST_FORMAT] = dst_fmt;

  O2EMU_LOG_DEBUG("ICE CSC convert: " << width << "x" << height
                                      << " fmt=" << src_fmt << "->" << dst_fmt);

  status_ &= ~STATUS_BUSY;
  status_ |= STATUS_CSC_DONE;
  interrupt_status_ |= INTR_CSC_DONE;
  busy_ = false;
}

void ICE::scale_image(u32 src_addr, u32 dst_addr, u32 src_w, u32 src_h,
                      u32 dst_w, u32 dst_h, Format fmt) {
  if (!enabled_)
    return;

  current_op_ = OP_SCALE;
  busy_ = true;
  status_ |= STATUS_BUSY;

  regs_[ICE_SRC_ADDR] = src_addr;
  regs_[ICE_DST_ADDR] = dst_addr;
  regs_[ICE_SRC_WIDTH] = src_w;
  regs_[ICE_SRC_HEIGHT] = src_h;
  regs_[ICE_DST_WIDTH] = dst_w;
  regs_[ICE_DST_HEIGHT] = dst_h;
  regs_[ICE_SRC_FORMAT] = fmt;
  regs_[ICE_DST_FORMAT] = fmt;

  float h_factor = static_cast<float>(src_w) / dst_w;
  float v_factor = static_cast<float>(src_h) / dst_h;
  regs_[ICE_SCALE_H_FACTOR] = *reinterpret_cast<u32 *>(&h_factor);
  regs_[ICE_SCALE_V_FACTOR] = *reinterpret_cast<u32 *>(&v_factor);

  O2EMU_LOG_DEBUG("ICE scale: " << src_w << "x" << src_h << " -> " << dst_w
                                << "x" << dst_h);

  status_ &= ~STATUS_BUSY;
  status_ |= STATUS_SCALE_DONE;
  interrupt_status_ |= INTR_SCALE_DONE;
  busy_ = false;
}

void ICE::set_jpeg_qtable(const u8 *qtable) {
  if (qtable) {
    std::memcpy(jpeg_qtable_.data(), qtable, 64);
  }
}

void ICE::set_jpeg_htable_dc(const u8 *htable) {
  if (htable) {
    std::memcpy(jpeg_htable_dc_.data(), htable, 256);
  }
}

void ICE::set_jpeg_htable_ac(const u8 *htable) {
  if (htable) {
    std::memcpy(jpeg_htable_ac_.data(), htable, 256);
  }
}

void ICE::set_csc_matrix(const float *matrix, const float *offset) {
  if (matrix) {
    std::memcpy(csc_matrix_, matrix, 9 * sizeof(float));
  }
  if (offset) {
    std::memcpy(csc_offset_, offset, 3 * sizeof(float));
  }
}

bool ICE::busy() const { return busy_; }

u32 ICE::status() const { return status_; }

u32 ICE::interrupt_status() const {
  return interrupt_status_ & interrupt_mask_;
}

void ICE::reset() {
  std::memset(regs_.data(), 0, regs_.size() * sizeof(u32));
  enabled_ = false;
  busy_ = false;
  status_ = STATUS_CMD_FIFO_EMPTY | STATUS_DATA_FIFO_EMPTY;
  control_ = 0;
  interrupt_mask_ = 0;
  interrupt_status_ = 0;
  fifo_head_ = 0;
  fifo_tail_ = 0;
  jpeg_qtable_.fill(0);
  jpeg_htable_dc_.fill(0);
  jpeg_htable_ac_.fill(0);
  std::memcpy(csc_matrix_, (float[9]){1, 0, 0, 0, 1, 0, 0, 0, 1},
              9 * sizeof(float));
  std::memset(csc_offset_, 0, 3 * sizeof(float));
  current_op_ = OP_NONE;
}

} // namespace o2emu::graphics