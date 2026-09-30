#pragma once

#include "audio/capture/ProcessCapturePool.hpp"
#include "audio/engine/AudioMixer.hpp"
#include "audio/engine/AudioPolicyEngine.hpp"
#include "audio/format/AudioFrame.hpp"
#include "policy/AudioPolicy.hpp"
#include "process/ProcessModel.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace shareguard {

class AudioEngine {
 public:
  using SnapshotHandler = std::function<ProcessSnapshot()>;
  using FrameHandler = std::function<void(const AudioFrame&)>;
  using ErrorHandler = std::function<void(const std::string& code, const std::string& message)>;

  ~AudioEngine();

  bool start(const AudioPolicy& policy, SnapshotHandler snapshot, FrameHandler onFrame, ErrorHandler onError,
             std::string& error);
  void stop();
  void applyPolicy(const AudioPolicy& policy);
  void noteSnapshot();
  bool capturing() const { return capturing_.load(); }
  const std::string& failureCode() const { return failureCode_; }
  CaptureStrategy strategy() const;
  int sourceCount() const { return sourceCount_.load(); }

 private:
  void loop(std::stop_token stop);
  void writeLoop(std::stop_token stop);
  bool rebuild(const ProcessSnapshot& snapshot, const AudioPolicy& policy, std::string& code, std::string& error);
  void enqueue(AudioFrame frame);
  void logDiagnostics(std::chrono::steady_clock::time_point& lastLog, std::uint64_t& lastCaptured, std::uint64_t& lastMixed,
                      std::uint64_t& lastSent);

  AudioPolicyEngine policyEngine_;
  AudioMixer mixer_;
  ProcessCapturePool pool_;
  std::unique_ptr<CaptureSource> single_;
  AudioPolicy policy_;
  CapturePlan plan_;
  SnapshotHandler snapshot_;
  FrameHandler onFrame_;
  ErrorHandler onError_;
  mutable std::mutex mutex_;
  std::mutex startMutex_;
  std::condition_variable startCv_;
  bool startFinished_ = false;
  bool startOk_ = false;
  std::string startError_;
  std::string failureCode_;
  std::atomic<bool> dirty_{false};
  std::atomic<bool> capturing_{false};
  std::atomic<int> sourceCount_{0};
  std::uint64_t sequence_ = 0;
  int rapidRestarts_ = 0;
  std::chrono::steady_clock::time_point openedAt_{};
  std::mutex queueMutex_;
  std::condition_variable_any queueReady_;
  std::deque<AudioFrame> queue_;
  std::atomic<std::uint64_t> mixedFrames_{0};
  std::atomic<std::uint64_t> sentFrames_{0};
  std::atomic<std::uint64_t> transportDrops_{0};
  std::atomic<std::uint64_t> starved_{0};
  std::atomic<int> queueFrames_{0};
  std::jthread writer_;
  std::jthread thread_;
};

}
