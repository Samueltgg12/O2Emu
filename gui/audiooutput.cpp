/**
 * @file audiooutput.cpp
 * @brief Qt6 audio sink implementation.
 */

#include "audiooutput.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>
#include <QMediaDevices>

AudioOutput::AudioOutput(QObject *parent) : QObject(parent) {}

AudioOutput::~AudioOutput() { stop(); }

bool AudioOutput::configure(int sample_rate, int channels) {
  if (sample_rate <= 0 || (channels != 1 && channels != 2)) {
    return false;
  }

  sample_rate_ = sample_rate;
  channels_ = channels;

  QAudioFormat format;
  format.setSampleRate(sample_rate_);
  format.setChannelCount(channels_);
  format.setSampleFormat(QAudioFormat::Int16);

  const QAudioDevice device = QMediaDevices::defaultAudioOutput();
  if (!device.isFormatSupported(format)) {
    // Fall back to the device's preferred format, keeping our rate/channels.
    format = device.preferredFormat();
    format.setSampleRate(sample_rate_);
    format.setChannelCount(channels_);
    if (!device.isFormatSupported(format)) {
      return false;
    }
  }

  // Recreate the sink with the new format.
  stop();
  sink_ = std::make_unique<QAudioSink>(device, format);
  return true;
}

void AudioOutput::start() {
  if (!sink_ || running_) {
    return;
  }
  device_ = sink_->start();
  if (device_) {
    running_ = true;
  }
}

void AudioOutput::stop() {
  if (sink_) {
    sink_->stop();
  }
  device_ = nullptr;
  running_ = false;
}

void AudioOutput::push(const int16_t *samples, size_t frames) {
  if (!running_ || !device_ || frames == 0) {
    return;
  }

  const size_t bytes =
      frames * static_cast<size_t>(channels_) * sizeof(int16_t);
  const char *data = reinterpret_cast<const char *>(samples);

  // Write as much as the device will accept. QIODevice::write may accept a
  // partial write; loop to drain the full block.
  size_t written = 0;
  while (written < bytes) {
    const qint64 n =
        device_->write(data + written, static_cast<qint64>(bytes - written));
    if (n <= 0) {
      break;
    }
    written += static_cast<size_t>(n);
  }
}