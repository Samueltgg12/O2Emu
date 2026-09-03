#pragma once

/**
 * @file mace_audio.h
 * @brief MACE Audio (AD1843 codec interface + MAVB)
 *
 * Offset from MACE base: 0x300000
 * Based on Linux sound/pci/sgio2audio.c and IRIX ad1843.h
 */

#include <array>
#include <o2emu/devices/device.h>
#include <o2emu/o2emu.h>

namespace o2emu::devices {

class MACEAudio : public Device {
public:
  MACEAudio();
  ~MACEAudio() override;

  // MACE Audio register offsets (from MACE base + 0x300000)
  enum Register : uint32_t {
    // Audio Control
    AUDIO_CTRL = 0x0000,
    AUDIO_STATUS = 0x0004,
    AUDIO_INT_MASK = 0x0008,
    AUDIO_INT_STATUS = 0x000C,

    // Playback DMA
    PLAY_DMA_BASE = 0x0100,
    PLAY_DMA_CUR = 0x0104,
    PLAY_DMA_END = 0x0108,
    PLAY_DMA_CTRL = 0x010C,
    PLAY_DMA_STATUS = 0x0110,

    // Capture DMA
    CAP_DMA_BASE = 0x0200,
    CAP_DMA_CUR = 0x0204,
    CAP_DMA_END = 0x0208,
    CAP_DMA_CTRL = 0x020C,
    CAP_DMA_STATUS = 0x0210,

    // MAVB (Multimedia Audio Video Bus) - for CD-ROM, etc.
    MAVB_CTRL = 0x0300,
    MAVB_STATUS = 0x0304,
    MAVB_DATA = 0x0308,

    // AD1843 Codec Control (indirect via MACE)
    CODEC_INDEX = 0x0400,
    CODEC_DATA = 0x0404,
    CODEC_STATUS = 0x0408,

    // Revision
    REVISION = 0x0FFC,
  };

  // AUDIO_CTRL bits
  enum AudioCtrlBit : uint32_t {
    AUDIO_CTRL_PLAY_EN = 0x00000001,
    AUDIO_CTRL_CAP_EN = 0x00000002,
    AUDIO_CTRL_LOOPBACK = 0x00000004,
    AUDIO_CTRL_MUTE = 0x00000008,
  };

  // AD1843 Codec registers (from IRIX ad1843.h)
  enum CodecRegister : uint8_t {
    CODEC_LEFT_ADC_IN = 0x00,
    CODEC_RIGHT_ADC_IN = 0x01,
    CODEC_LEFT_AUX1_IN = 0x02,
    CODEC_RIGHT_AUX1_IN = 0x03,
    CODEC_LEFT_AUX2_IN = 0x04,
    CODEC_RIGHT_AUX2_IN = 0x05,
    CODEC_LEFT_DAC_OUT = 0x06,
    CODEC_RIGHT_DAC_OUT = 0x07,
    CODEC_LEFT_MASTER = 0x08,
    CODEC_RIGHT_MASTER = 0x09,
    CODEC_MONO_OUT = 0x0A,
    CODEC_MONO_IN = 0x0B,
    CODEC_MIC_GAIN = 0x0C,
    CODEC_REC_SELECT = 0x0D,
    CODEC_REC_GAIN = 0x0E,
    CODEC_GENERAL_PURPOSE = 0x0F,
    CODEC_3D_CONTROL = 0x10,
    CODEC_POWER_DOWN = 0x11,
    CODEC_DIGITAL_IF = 0x12,
    CODEC_SERIAL_CFG = 0x13,
    CODEC_MISC = 0x14,
    CODEC_DIGITAL_PIN = 0x15,
    CODEC_ID = 0x1E,
    CODEC_TEST = 0x1F,
  };

  // Device interface
  u32 read32(u32 offset) override;
  u16 read16(u32 offset) override;
  u8 read8(u32 offset) override;

  void write32(u32 offset, u32 value) override;
  void write16(u32 offset, u16 value) override;
  void write8(u32 offset, u8 value) override;

  void reset() override;
  void tick(u64 cycles) override;

  // Codec access
  u16 codec_read(CodecRegister reg);
  void codec_write(CodecRegister reg, u16 value);

  // Audio callbacks (for integration with audio backend)
  using AudioCallback =
      std::function<void(const int16_t *samples, size_t frames, int channels)>;
  void set_playback_callback(AudioCallback cb) { playback_cb_ = std::move(cb); }
  void set_capture_callback(AudioCallback cb) { capture_cb_ = std::move(cb); }

  // Sample rate / format
  void set_sample_rate(u32 rate) { sample_rate_ = rate; }
  u32 sample_rate() const { return sample_rate_; }

  void set_format(u32 bits, u32 channels) {
    bits_ = bits;
    channels_ = channels;
  }

private:
  std::array<u32, 0x1000 / 4> regs_ = {};

  // AD1843 codec registers
  std::array<u16, 32> codec_regs_ = {};

  // DMA state
  bool play_dma_active_ = false;
  bool cap_dma_active_ = false;
  u32 play_dma_pos_ = 0;
  u32 cap_dma_pos_ = 0;

  // Audio format
  u32 sample_rate_ = 48000;
  u32 bits_ = 16;
  u32 channels_ = 2;

  // Audio callbacks
  AudioCallback playback_cb_;
  AudioCallback capture_cb_;

  // Audio buffers (for DMA simulation)
  std::vector<int16_t> play_buffer_;
  std::vector<int16_t> cap_buffer_;
};

} // namespace o2emu::devices