#pragma once

#include <cstddef>
#include <mutex>
#include <vector>

namespace shareguard {

class AudioRingBuffer {
 public:
  explicit AudioRingBuffer(size_t capacityFrames = 12000);

  void push(const float* interleavedStereo, size_t frames);
  size_t pull(float* interleavedStereo, size_t frames);
  void clear();

 private:
  mutable std::mutex mutex_;
  std::vector<float> data_;
  size_t capacityFrames_ = 0;
  size_t readFrame_ = 0;
  size_t availableFrames_ = 0;
};

}
