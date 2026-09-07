/**
 * @file mace_audio.cpp
 * @brief MACE Audio implementation
 */

#include <o2emu/devices/mace/mace.h>
#include <o2emu/devices/mace/mace_audio.h>
#include <o2emu/logging/logger.h>
#include <o2emu/memory/memory.h>

#include <cstring>
#include <vector>

namespace o2emu::devices {

MACEAudio::MACEAudio(MACE &mace) : mace_(mace) { reset(); }

void MACEAudio::reset() {
  regs_.fill(0);

  // MACE Audio register defaults (based on CS4215/CS4231 compatible)
  // AUDIO_CTRL: 0x0000
  regs_[REG_CTRL] = 0x00000000;

  // AUDIO_STATUS: 0x0008
  regs_[REG_STATUS] = 0x00000000;

  // AUDIO_FORMAT: 0x0010
  regs_[REG_FORMAT] = 0x00000000;

  // AUDIO_SAMPLE_RATE: 0x0018
  regs_[REG_SAMPLE_RATE] = 0x0000AC44; // 44.1kHz

  // AUDIO_PLAY_BASE: 0x0100
  regs_[REG_PLAY_BASE] = 0x00000000;

  // AUDIO_PLAY_PTR: 0x0108
  regs_[REG_PLAY_PTR] = 0x00000000;

  // AUDIO_PLAY_END: 0x0110
  regs_[REG_PLAY_END] = 0x00000000;

  // AUDIO_PLAY_CTRL: 0x0118
  regs_[REG_PLAY_CTRL] = 0x00000000;

  // AUDIO_REC_BASE: 0x0200
  regs_[REG_REC_BASE] = 0x00000000;

  // AUDIO_REC_PTR: 0x0208
  regs_[REG_REC_PTR] = 0x00000000;

  // AUDIO_REC_END: 0x0210
  regs_[REG_REC_END] = 0x00000000;

  // AUDIO_REC_CTRL: 0x0218
  regs_[REG_REC_CTRL] = 0x00000000;

  // AUDIO_VOLUME: 0x0300
  regs_[REG_VOLUME] = 0x00008080; // 50% volume

  // AUDIO_INT_STATUS: 0x0400
  regs_[REG_INT_STATUS] = 0x00000000;

  // AUDIO_INT_MASK: 0x0408
  regs_[REG_INT_MASK] = 0x00000000;

  // AUDIO_INT_CLEAR: 0x0410
  regs_[REG_INT_CLEAR] = 0x00000000;
}

u32 MACEAudio::read_reg(u32 offset) {
  if (offset < sizeof(regs_)) {
    return regs_[offset / 4];
  }
  return 0;
}

void MACEAudio::write_reg(u32 offset, u32 value) {
  if (offset >= sizeof(regs_))
    return;

  u32 reg = offset / 4;

  switch (reg) {
  case REG_CTRL:
    regs_[reg] = value;
    break;

  case REG_FORMAT:
    regs_[reg] = value;
    break;

  case REG_SAMPLE_RATE:
    regs_[reg] = value;
    break;

  case REG_PLAY_BASE:
  case REG_PLAY_PTR:
  case REG_PLAY_END:
  case REG_PLAY_CTRL:
    regs_[reg] = value;
    if (reg == REG_PLAY_CTRL && (value & 0x1)) {
      start_playback();
    }
    break;

  case REG_REC_BASE:
  case REG_REC_PTR:
  case REG_REC_END:
  case REG_REC_CTRL:
    regs_[reg] = value;
    if (reg == REG_REC_CTRL && (value & 0x1)) {
      start_record();
    }
    break;

  case REG_VOLUME:
    regs_[reg] = value;
    break;

  case REG_INT_MASK:
    regs_[reg] = value;
    break;

  case REG_INT_CLEAR:
    regs_[REG_INT_STATUS] &= ~value;
    break;

  default:
    regs_[reg] = value;
    break;
  }
}

void MACEAudio::start_playback() {
  u32 play_base = regs_[REG_PLAY_BASE];
  u32 play_ptr = regs_[REG_PLAY_PTR];
  u32 play_end = regs_[REG_PLAY_END];

  O2EMU_LOG_DEBUG_F("Audio playback start: base=0x%X ptr=0x%X end=0x%X",
                    play_base, play_ptr, play_end);

  // If the DMA region is empty or the pointer is already at the end, mark
  // playback complete without emitting samples.
  if (play_end <= play_base || play_ptr >= play_end) {
    regs_[REG_PLAY_PTR] = play_end;
    regs_[REG_INT_STATUS] |= INT_PLAY_DONE;
    return;
  }

  // Decode the format: 8/16-bit, mono/stereo.
  u32 format = regs_[REG_FORMAT];
  bool is_16bit = (format & FORMAT_16BIT) != 0;
  bool is_stereo = (format & FORMAT_MONO) == 0;

  // Read the raw DMA buffer from emulated memory.
  size_t byte_len = play_end - play_ptr;
  std::vector<u8> raw(byte_len);
  mace_.memory().read_block(play_ptr, raw.data(), byte_len);

  // Convert to interleaved 16-bit signed PCM.
  std::vector<int16_t> pcm;
  if (is_16bit) {
    // 16-bit samples are stored little-endian in memory.
    size_t samples = byte_len / 2;
    pcm.resize(samples);
    for (size_t i = 0; i < samples; ++i) {
      pcm[i] = static_cast<int16_t>(raw[i * 2] | (raw[i * 2 + 1] << 8));
    }
  } else {
    // 8-bit samples are unsigned (0..255), centered at 128.
    pcm.resize(byte_len);
    for (size_t i = 0; i < byte_len; ++i) {
      pcm[i] = static_cast<int16_t>((static_cast<int>(raw[i]) - 128) << 8);
    }
  }

  size_t channels = is_stereo ? 2 : 1;
  size_t frames = pcm.size() / channels;

  if (sample_cb_ && frames > 0) {
    sample_cb_(pcm.data(), frames);
  }

  // Advance the DMA pointer and signal completion.
  regs_[REG_PLAY_PTR] = play_end;
  regs_[REG_INT_STATUS] |= INT_PLAY_DONE;
}

void MACEAudio::start_record() {
  u32 rec_base = regs_[REG_REC_BASE];
  u32 rec_ptr = regs_[REG_REC_PTR];
  u32 rec_end = regs_[REG_REC_END];

  O2EMU_LOG_DEBUG_F("Audio record start: base=0x%X ptr=0x%X end=0x%X", rec_base,
                    rec_ptr, rec_end);

  // Simulate record completion
  regs_[REG_REC_PTR] = rec_end;
  regs_[REG_INT_STATUS] |= 0x2; // Record complete
}

bool MACEAudio::read(u32 offset, u32 size, u32 &value) {
  [[maybe_unused]] auto sz = size;
  if (offset < sizeof(regs_)) {
    value = read_reg(offset);
    return true;
  }
  return false;
}

bool MACEAudio::write(u32 offset, u32 size, u32 value) {
  [[maybe_unused]] auto sz = size;
  if (offset < sizeof(regs_)) {
    write_reg(offset, value);
    return true;
  }
  return false;
}

void MACEAudio::tick([[maybe_unused]] u64 cycles) {
  // If playback is enabled and the DMA pointer has not yet reached the end,
  // continue streaming the remaining samples. This supports drivers that
  // program a large buffer and let the DMA engine advance the pointer.
  if ((regs_[REG_PLAY_CTRL] & DMA_CTRL_ENABLE) &&
      !(regs_[REG_PLAY_CTRL] & DMA_CTRL_PAUSE)) {
    u32 play_ptr = regs_[REG_PLAY_PTR];
    u32 play_end = regs_[REG_PLAY_END];
    if (play_ptr < play_end) {
      start_playback();
    }
  }
}

} // namespace o2emu::devices