#include "audio/format/AudioConverter.hpp"

#include <algorithm>
#include <cstring>

namespace shareguard {
namespace {

bool audioGuid(const GUID& guid, unsigned long format) {
  const GUID pcm = {format, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
  return IsEqualGUID(guid, pcm);
}

}

bool AudioConverter::prepare(const WAVEFORMATEX* format, std::string& error) {
  reset();
  sourceRate_ = 0;
  if (format == nullptr) {
    error = "Audio format is missing.";
    return false;
  }
  sourceRate_ = static_cast<int>(format->nSamplesPerSec);
  sourceChannels_ = format->nChannels;
  containerBits_ = format->wBitsPerSample;
  blockAlign_ = format->nBlockAlign;
  isFloat_ = false;
  if (sourceChannels_ <= 0 || sourceRate_ <= 0 || blockAlign_ <= 0) {
    error = "Audio format is invalid.";
    sourceRate_ = 0;
    return false;
  }

  if (format->wFormatTag == WAVE_FORMAT_PCM && (containerBits_ == 16 || containerBits_ == 24 || containerBits_ == 32)) {
    isFloat_ = false;
  } else if (format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT && containerBits_ == 32) {
    isFloat_ = true;
  } else if (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE && format->cbSize >= 22) {
    const auto* extensible = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(format);
    if (audioGuid(extensible->SubFormat, 0x00000003) && format->wBitsPerSample == 32) {
      isFloat_ = true;
    } else if (audioGuid(extensible->SubFormat, 0x00000001) &&
               (format->wBitsPerSample == 16 || format->wBitsPerSample == 24 || format->wBitsPerSample == 32)) {
      isFloat_ = false;
    } else {
      error = "Unsupported extensible audio format.";
      sourceRate_ = 0;
      return false;
    }
  } else {
    error = "Unsupported audio format.";
    sourceRate_ = 0;
    return false;
  }
  return true;
}

void AudioConverter::reset() {
  fifo_.clear();
  position_ = 0.0;
}

float AudioConverter::readSample(const uint8_t* frame, int channel) const {
  const int stride = containerBits_ / 8;
  const uint8_t* sample = frame + channel * stride;
  if (isFloat_) {
    float value = 0.0f;
    std::memcpy(&value, sample, sizeof(value));
    return value;
  }
  if (containerBits_ == 16) {
    int16_t value = 0;
    std::memcpy(&value, sample, sizeof(value));
    return static_cast<float>(value) / 32768.0f;
  }
  if (containerBits_ == 24) {
    int32_t value = sample[0] | (sample[1] << 8) | (sample[2] << 16);
    if (value & 0x800000) {
      value |= ~0xFFFFFF;
    }
    return static_cast<float>(value) / 8388608.0f;
  }
  int32_t value = 0;
  std::memcpy(&value, sample, sizeof(value));
  return static_cast<float>(value) / 2147483648.0f;
}

void AudioConverter::appendSourceFrame(float left, float right, std::vector<float>& output) {
  if (sourceRate_ == 48000) {
    output.push_back(left);
    output.push_back(right);
    return;
  }

  fifo_.push_back(left);
  fifo_.push_back(right);
  const double step = static_cast<double>(sourceRate_) / 48000.0;
  const size_t availableFrames = fifo_.size() / 2;
  while (position_ + 1.0 < static_cast<double>(availableFrames)) {
    const size_t index = static_cast<size_t>(position_);
    const float fraction = static_cast<float>(position_ - static_cast<double>(index));
    const float leftSample = fifo_[index * 2] + (fifo_[(index + 1) * 2] - fifo_[index * 2]) * fraction;
    const float rightSample =
        fifo_[index * 2 + 1] + (fifo_[(index + 1) * 2 + 1] - fifo_[index * 2 + 1]) * fraction;
    output.push_back(leftSample);
    output.push_back(rightSample);
    position_ += step;
  }

  const size_t drop = static_cast<size_t>(position_);
  if (drop > 0) {
    fifo_.erase(fifo_.begin(), fifo_.begin() + static_cast<std::ptrdiff_t>(drop * 2));
    position_ -= static_cast<double>(drop);
  }
  if (fifo_.size() > 48000 * 2) {
    fifo_.erase(fifo_.begin(), fifo_.end() - 4800);
    position_ = 0.0;
  }
}

void AudioConverter::convert(const BYTE* data, UINT32 frames, bool silent, std::vector<float>& interleavedStereo) {
  if (sourceRate_ == 0 || frames == 0) {
    return;
  }
  if (silent || data == nullptr) {
    for (UINT32 frame = 0; frame < frames; ++frame) {
      appendSourceFrame(0.0f, 0.0f, interleavedStereo);
    }
    return;
  }

  const auto* bytes = reinterpret_cast<const uint8_t*>(data);
  for (UINT32 frame = 0; frame < frames; ++frame) {
    const uint8_t* frameBytes = bytes + static_cast<size_t>(frame) * static_cast<size_t>(blockAlign_);
    const float left = readSample(frameBytes, 0);
    const float right = sourceChannels_ >= 2 ? readSample(frameBytes, 1) : left;
    appendSourceFrame(left, right, interleavedStereo);
  }
}

}
