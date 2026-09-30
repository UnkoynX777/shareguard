#include "audio/buffer/AudioRingBuffer.hpp"

#include <algorithm>
#include <cstring>

namespace shareguard {

AudioRingBuffer::AudioRingBuffer(size_t capacityFrames) : capacityFrames_(capacityFrames), data_(capacityFrames * 2) {}

void AudioRingBuffer::push(const float* interleavedStereo, size_t frames) {
  if (interleavedStereo == nullptr || frames == 0 || capacityFrames_ == 0) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (frames >= capacityFrames_) {
    interleavedStereo += (frames - capacityFrames_) * 2;
    frames = capacityFrames_;
    readFrame_ = 0;
    availableFrames_ = 0;
  }
  const size_t overflow =
      availableFrames_ + frames > capacityFrames_ ? availableFrames_ + frames - capacityFrames_ : 0;
  if (overflow > 0) {
    readFrame_ = (readFrame_ + overflow) % capacityFrames_;
    availableFrames_ -= overflow;
    droppedFrames_.fetch_add(overflow);
  }
  size_t remaining = frames;
  const float* input = interleavedStereo;
  while (remaining > 0) {
    const size_t writeFrame = (readFrame_ + availableFrames_) % capacityFrames_;
    const size_t run = std::min(remaining, capacityFrames_ - writeFrame);
    std::memcpy(data_.data() + writeFrame * 2, input, run * 2 * sizeof(float));
    availableFrames_ += run;
    input += run * 2;
    remaining -= run;
  }
  ready_.notify_all();
}

size_t AudioRingBuffer::available() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return availableFrames_;
}

bool AudioRingBuffer::waitFor(size_t minFrames, std::stop_token stop, std::chrono::milliseconds timeout) {
  std::unique_lock<std::mutex> lock(mutex_);
  return ready_.wait_for(lock, stop, timeout, [&] { return availableFrames_ >= minFrames; });
}

size_t AudioRingBuffer::pull(float* interleavedStereo, size_t frames) {
  if (interleavedStereo == nullptr || frames == 0) {
    return 0;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  const size_t count = std::min(frames, availableFrames_);
  size_t remaining = count;
  float* output = interleavedStereo;
  while (remaining > 0) {
    const size_t run = std::min(remaining, capacityFrames_ - readFrame_);
    std::memcpy(output, data_.data() + readFrame_ * 2, run * 2 * sizeof(float));
    readFrame_ = (readFrame_ + run) % capacityFrames_;
    availableFrames_ -= run;
    output += run * 2;
    remaining -= run;
  }
  return count;
}

void AudioRingBuffer::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  readFrame_ = 0;
  availableFrames_ = 0;
}

}
