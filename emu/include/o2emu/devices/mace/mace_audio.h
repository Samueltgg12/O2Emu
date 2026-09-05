#pragma once

/**
 * @file mace_audio.h
 * @brief MACE Audio (AD1843/CS4215 compatible)
 *
 * Offset from MACE base: 0x300000
 * Based on Linux sound/pci/sgio2audio.c and IRIX ad1843.h
 */

#include <array>
#include <o2emu/devices/device.h>
#include <o2emu/o2emu.h>

namespace o2emu::devices {

class MACE;

class MACEAudio {
public:
  explicit MACEAudio(MACE &mace);
  ~MACEAudio() = default;

  // MACE Audio register offsets (from MACE base + 0x300000)
  enum Register : uint32_t {
    REG_CTRL = 0x0000,
    REG_STATUS = 0x0004,
    REG_FORMAT = 0x0010,
    REG_SAMPLE_RATE = 0x0018,
    REG_PLAY_BASE = 0x0100,
    REG_PLAY_PTR = 0x0108,
    REG_PLAY_END = 0x0110,
    REG_PLAY_CTRL = 0x0118,
    REG_REC_BASE = 0x0200,
    REG_REC_PTR = 0x0208,
    REG_REC_END = 0x0210,
    REG_REC_CTRL = 0x0218,
    REG_VOLUME = 0x0300,
    REG_INT_STATUS = 0x0400,
    REG_INT_MASK = 0x0408,
    REG_INT_CLEAR = 0x0410,
    REG_REVISION = 0x0FFC,
    SIZE = 0x1000,
  };

  // REG_CTRL bits
  enum CtrlBit : uint32_t {
    CTRL_PLAY_EN = 0x00000001,
    CTRL_REC_EN = 0x00000002,
    CTRL_LOOPBACK = 0x00000004,
    CTRL_MUTE = 0x00000008,
  };

  // REG_FORMAT bits
  enum FormatBit : uint32_t {
    FORMAT_8BIT = 0x00000000,
    FORMAT_16BIT = 0x00000001,
    FORMAT_STEREO = 0x00000000,
    FORMAT_MONO = 0x00000002,
  };

  // REG_PLAY_CTRL / REG_REC_CTRL bits
  enum DmaCtrlBit : uint32_t {
    DMA_CTRL_ENABLE = 0x00000001,
    DMA_CTRL_IRQ_EN = 0x00000002,
    DMA_CTRL_PAUSE = 0x00000004,
  };

  // REG_INT_STATUS / REG_INT_MASK bits
  enum IntBit : uint32_t {
    INT_PLAY_DONE = 0x00000001,
    INT_REC_DONE = 0x00000002,
    INT_PLAY_UNDERRUN = 0x00000004,
    INT_REC_OVERRUN = 0x00000008,
  };

  // Device interface
  u32 read_reg(u32 offset);
  void write_reg(u32 offset, u32 value);

  bool read(u32 offset, u32 size, u32 &value);
  bool write(u32 offset, u32 size, u32 value);

  void reset();
  void tick(u64 cycles);
  u32 interrupt_status() const;

  // Audio control
  void start_playback();
  void start_record();

private:
  MACE &mace_;
  std::array<u32, SIZE / 4> regs_ = {};
};

} // namespace o2emu::devices