/**
 * @file mace_audio.cpp
 * @brief MACE Audio implementation
 */

#include <cstring>
#include <o2emu/devices/mace/mace_audio.h>
#include <o2emu/logging/logger.h>

namespace o2emu::devices {

MACEAudio::MACEAudio(MACE &mace) : mace_(mace) { reset(); }

MACEAudio::~MACEAudio() = default;

void MACEAudio::reset() {
  std::memset(regs_, 0, sizeof(regs_));

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

  O2EMU_LOG_DEBUG("Audio playback start: base=0x"
                  << std::hex << play_base << " ptr=0x" << play_ptr << " end=0x"
                  << play_end << std::dec);

  // Simulate playback completion
  regs_[REG_PLAY_PTR] = play_end;
  regs_[REG_INT_STATUS] |= 0x1; // Playback complete
}

void MACEAudio::start_record() {
  u32 rec_base = regs_[REG_REC_BASE];
  u32 rec_ptr = regs_[REG_REC_PTR];
  u32 rec_end = regs_[REG_REC_END];

  O2EMU_LOG_DEBUG("Audio record start: base=0x"
                  << std::hex << rec_base << " ptr=0x" << rec_ptr << " end=0x"
                  << rec_end << std::dec);

  // Simulate record completion
  regs_[REG_REC_PTR] = rec_end;
  regs_[REG_INT_STATUS] |= 0x2; // Record complete
}

bool MACEAudio::read(u32 offset, u32 size, u32 &value) {
  if (offset < sizeof(regs_)) {
    value = read_reg(offset);
    return true;
  }
  return false;
}

bool MACEAudio::write(u32 offset, u32 size, u32 value) {
  if (offset < sizeof(regs_)) {
    write_reg(offset, value);
    return true;
  }
  return false;
}

void MACEAudio::tick(u64 cycles) {
  // Audio doesn't need periodic updates in this simple implementation
}

u32 MACEAudio::interrupt_status() const {
  return regs_[REG_INT_STATUS] & regs_[REG_INT_MASK];
}

} // namespace o2emu::devices