#pragma once

#include "audio/capture/ProcessCapturePool.hpp"
#include "audio/engine/AudioMixer.hpp"
#include "audio/engine/AudioPolicyEngine.hpp"
#include "audio/engine/PolicyMailbox.hpp"
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
#include <vector>

namespace shareguard {

struct AudioGraph {
  CaptureStrategy strategy = CaptureStrategy::SystemLoopback;
  std::uint64_t generation = 0;
  std::uint32_t excludeRootPid = 0;
  std::vector<std::shared_ptr<CaptureSource>> sources;
  std::vector<AudioRingBuffer*> inputs;
};

class AudioEngine {
 public:
  using SnapshotHandler = std::function<ProcessSnapshot()>;
  using FrameHandler = std::function<void(const AudioFrame&)>;
  using ErrorHandler = std::function<void(const std::string& code, const std::string& message)>;
  using AppliedHandler = std::function<void(std::uint64_t revision, CaptureStrategy strategy, int blockedCount)>;

  ~AudioEngine();

  bool start(const AudioPolicy& policy, SnapshotHandler snapshot, FrameHandler onFrame, ErrorHandler onError,
             std::string& error);
  void stop();
  std::uint64_t applyPolicy(const AudioPolicy& policy);
  void noteSnapshot();
  void setAppliedHandler(AppliedHandler handler);
  bool capturing() const { return capturing_.load(); }
  const std::string& failureCode() const { return failureCode_; }
  CaptureStrategy strategy() const;
  int sourceCount() const { return sourceCount_.load(); }

 private:
  void loop(std::stop_token stop);
  void writeLoop(std::stop_token stop);
  void controlLoop(std::stop_token stop);
  bool reconcile(std::stop_token stop, std::string& error);
  void publish(const std::shared_ptr<AudioGraph>& graph, const char* reason);
  void reap();
  bool openDirect(bool exclude, std::uint32_t pid, std::string& error);
  bool openMix(const std::vector<std::uint32_t>& rootPids, std::string& error);
  void markApplied(std::uint64_t revision, CaptureStrategy strategy, int blockedCount);
  void checkHealth();
  std::shared_ptr<AudioGraph> makeGraph(CaptureStrategy strategy, std::vector<std::shared_ptr<CaptureSource>> sources) const;
  void enqueue(AudioFrame frame);
  void logDiagnostics(std::chrono::steady_clock::time_point& lastLog, std::uint64_t& lastCaptured, std::uint64_t& lastMixed,
                      std::uint64_t& lastSent);

  AudioPolicyEngine policyEngine_;
  PolicyMailbox mailbox_;
  AudioMixer mixer_;
  ProcessCapturePool pool_;
  std::shared_ptr<CaptureSource> direct_;
  CaptureStrategy strategyFloor_ = CaptureStrategy::SystemLoopback;
  CapturePlan plan_;
  SnapshotHandler snapshot_;
  FrameHandler onFrame_;
  ErrorHandler onError_;
  AppliedHandler onApplied_;
  mutable std::mutex mutex_;
  std::mutex startMutex_;
  std::condition_variable startCv_;
  bool startFinished_ = false;
  bool startOk_ = false;
  std::string startError_;
  std::string failureCode_;
  std::mutex controlMutex_;
  std::condition_variable_any controlCv_;
  std::atomic<bool> dirty_{false};
  std::atomic<bool> capturing_{false};
  std::atomic<bool> live_{false};
  std::atomic<int> sourceCount_{0};
  std::atomic<std::uint64_t> sessionId_{0};
  std::atomic<std::uint64_t> generation_{0};
  std::atomic<std::uint64_t> appliedRevision_{0};
  std::uint64_t notifiedRevision_ = 0;
  std::uint64_t sequence_ = 0;
  std::atomic<std::shared_ptr<AudioGraph>> published_;
  std::vector<std::shared_ptr<AudioGraph>> retired_;
  std::mutex queueMutex_;
  std::condition_variable_any queueReady_;
  std::deque<AudioFrame> queue_;
  std::atomic<std::uint64_t> mixedFrames_{0};
  std::atomic<std::uint64_t> sentFrames_{0};
  std::atomic<std::uint64_t> transportDrops_{0};
  std::atomic<std::uint64_t> starved_{0};
  std::atomic<std::uint64_t> incremental_{0};
  std::atomic<std::uint64_t> fullRebuilds_{0};
  std::atomic<std::uint64_t> expectedStops_{0};
  std::atomic<std::uint64_t> unexpectedStops_{0};
  std::atomic<std::uint64_t> staleIgnored_{0};
  std::atomic<int> queueFrames_{0};
  std::jthread control_;
  std::jthread writer_;
  std::jthread thread_;
};

}
