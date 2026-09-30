#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <stop_token>
#include <vector>

namespace shareguard {

class AudioRingBuffer {
 public:
  explicit AudioRingBuffer(size_t capacityFrames = 24000);

  void push(const float* interleavedStereo, size_t frames);
  size_t pull(float* interleavedStereo, size_t frames);
  size_t available() const;
  bool waitFor(size_t minFrames, std::stop_token stop, std::chrono::milliseconds timeout);
  std::uint64_t droppedFrames() const { return droppedFrames_.load(); }
  void clear();

 private:
  mutable std::mutex mutex_;
  std::condition_variable_any ready_;
  std::vector<float> data_;
  size_t capacityFrames_ = 0;
  size_t readFrame_ = 0;
  size_t availableFrames_ = 0;
  std::atomic<std::uint64_t> droppedFrames_{0};
};

}
