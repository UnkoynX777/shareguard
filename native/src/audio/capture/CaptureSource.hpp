#pragma once

#include "audio/buffer/AudioRingBuffer.hpp"
#include "audio/capture/LoopbackCapture.hpp"
#include "audio/format/AudioConverter.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace shareguard {

class CaptureSource {
 public:
  ~CaptureSource();

  bool startSystem(std::string& error);
  bool startProcess(std::uint32_t processId, bool excludeTree, std::string& error);
  AudioRingBuffer& buffer() { return buffer_; }
  bool running() const { return running_.load(); }
  std::uint64_t capturedFrames() const { return capturedFrames_.load(); }
  std::uint64_t discontinuities() const { return discontinuities_.load(); }
  int sampleRate() const { return sampleRate_.load(); }
  void stop();

 private:
  struct OpenResult {
    std::mutex mutex;
    std::condition_variable ready;
    bool finished = false;
    bool ok = false;
    std::string error;
  };

  bool start(bool process, std::uint32_t processId, bool excludeTree, std::string& error);
  void run(std::stop_token stop, bool process, std::uint32_t processId, bool excludeTree,
           const std::shared_ptr<OpenResult>& open);

  WasapiLoopbackSession session_;
  AudioConverter converter_;
  AudioRingBuffer buffer_;
  std::vector<float> convertScratch_;
  std::atomic<bool> running_{false};
  std::atomic<std::uint64_t> capturedFrames_{0};
  std::atomic<std::uint64_t> discontinuities_{0};
  std::atomic<int> sampleRate_{0};
  bool lastDeviceValid_ = false;
  UINT64 lastDeviceEnd_ = 0;
  std::jthread thread_;
};

}
