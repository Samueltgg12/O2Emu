/**
 * @file audiooutput.h
 * @brief Qt6 audio sink that streams MACE audio PCM to the host OS.
 *
 * Uses QAudioSink (Qt6 Multimedia) to push decoded 16-bit signed PCM samples
 * to the platform audio backend (ALSA/PipeWire/PulseAudio on Linux, WASAPI on
 * Windows, CoreAudio on macOS).
 */

#pragma once

#include <QObject>
#include <cstddef>
#include <cstdint>
#include <memory>

class QAudioSink;
class QIODevice;

class AudioOutput : public QObject {
  Q_OBJECT

public:
  explicit AudioOutput(QObject *parent = nullptr);
  ~AudioOutput() override;

  // Configure the sink for the given sample rate and channel count. Must be
  // called before start(). Returns false if the format is unsupported.
  bool configure(int sample_rate, int channels);

  // Begin streaming. Safe to call multiple times.
  void start();

  // Stop streaming and release the audio device.
  void stop();

  // Push a block of interleaved 16-bit signed PCM samples. `frames` is the
  // number of frames (one frame = one sample per channel).
  void push(const int16_t *samples, size_t frames);

  bool isRunning() const { return running_; }

private:
  std::unique_ptr<QAudioSink> sink_;
  QIODevice *device_ = nullptr;
  int sample_rate_ = 44100;
  int channels_ = 2;
  bool running_ = false;
};