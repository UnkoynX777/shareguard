#include "audio/engine/AudioEngine.hpp"

#include "logging/Logger.hpp"
#include "platform/windows/ComRuntime.hpp"
#include "platform/windows/WindowsVersion.hpp"

#include <cstring>

namespace shareguard {

AudioEngine::~AudioEngine() { stop(); }

bool AudioEngine::start(const AudioPolicy& policy, SnapshotHandler snapshot, FrameHandler onFrame, ErrorHandler onError,
                        std::string& error) {
  stop();
  failureCode_.clear();
  {
    policy_ = policy;
    snapshot_ = std::move(snapshot);
    onFrame_ = std::move(onFrame);
    onError_ = std::move(onError);
    plan_ = {};
    sequence_ = 0;
    rapidRestarts_ = 0;
    openedAt_ = {};
  }
  {
    std::lock_guard<std::mutex> lock(startMutex_);
    startFinished_ = false;
    startOk_ = false;
    startError_.clear();
  }
  dirty_.store(true);
  capturing_.store(true);
  thread_ = std::jthread([this](std::stop_token stop) { loop(stop); });
  std::unique_lock<std::mutex> lock(startMutex_);
  if (!startCv_.wait_for(lock, std::chrono::seconds(12), [&] { return startFinished_; })) {
    lock.unlock();
    error = "Timed out starting audio capture.";
    stop();
    return false;
  }
  if (!startOk_) {
    error = startError_.empty() ? "Audio capture failed." : startError_;
    lock.unlock();
    stop();
    return false;
  }
  return true;
}

void AudioEngine::stop() {
  capturing_.store(false);
  if (thread_.joinable()) {
    thread_.request_stop();
    thread_.join();
  }
  single_.reset();
  pool_.stop();
  sourceCount_.store(0);
}

void AudioEngine::applyPolicy(const AudioPolicy& policy) {
  std::lock_guard<std::mutex> lock(mutex_);
  policy_ = policy;
  dirty_.store(true);
}

void AudioEngine::noteSnapshot() { dirty_.store(true); }

CaptureStrategy AudioEngine::strategy() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return plan_.strategy;
}

void AudioEngine::loop(std::stop_token stop) {
  ComRuntime com;
  bool announced = false;
  int ticks = 0;
  auto announce = [&](bool ok, const std::string& message) {
    if (announced) {
      return;
    }
    std::lock_guard<std::mutex> lock(startMutex_);
    startOk_ = ok;
    startError_ = message;
    startFinished_ = true;
    startCv_.notify_all();
    announced = true;
  };

  while (!stop.stop_requested()) {
    if (dirty_.exchange(false)) {
      ProcessSnapshot snapshot = snapshot_ ? snapshot_() : ProcessSnapshot{};
      AudioPolicy policy;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        policy = policy_;
      }
      std::string code;
      std::string error;
      if (!rebuild(snapshot, policy, code, error)) {
        capturing_.store(false);
        failureCode_ = code.empty() ? "AUDIO_INITIALIZATION_FAILED" : code;
        if (announced && onError_) {
          onError_(failureCode_, error);
        }
        announce(false, error);
        break;
      }
      announce(true, {});
    }

    float block[kPacketFrames * 2];
    std::vector<AudioRingBuffer*> inputs;
    if (single_ && single_->running()) {
      inputs.push_back(&single_->buffer());
    } else {
      inputs = pool_.buffers();
    }
    mixer_.mix(inputs, block, kPacketFrames);
    if (onFrame_) {
      AudioFrame frame;
      frame.sequence = ++sequence_;
      quantizeStereo(block, kPacketFrames, frame.pcm);
      onFrame_(frame);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    if (++ticks % 50 == 0 || (single_ && !single_->running() && plan_.strategy != CaptureStrategy::AllowedProcessMix)) {
      dirty_.store(true);
    }
  }

  announce(false, "Capture stopped.");
  single_.reset();
  pool_.stop();
  capturing_.store(false);
}

bool AudioEngine::rebuild(const ProcessSnapshot& snapshot, const AudioPolicy& policy, std::string& code,
                          std::string& error) {
  const CapturePlan next = policyEngine_.plan(snapshot, policy);
  const bool same = next == plan_;
  if (same && next.strategy == CaptureStrategy::AllowedProcessMix && pool_.matches(next.includeRootPids)) {
    return true;
  }
  if (same && next.strategy != CaptureStrategy::AllowedProcessMix && single_ && single_->running()) {
    return true;
  }

  const auto now = std::chrono::steady_clock::now();
  if (openedAt_.time_since_epoch().count() != 0 && now - openedAt_ < std::chrono::seconds(2)) {
    ++rapidRestarts_;
  } else {
    rapidRestarts_ = 0;
  }
  if (rapidRestarts_ > 5) {
    code = "CAPTURE_STOPPED_UNEXPECTEDLY";
    error = "Capture stopped unexpectedly.";
    return false;
  }

  single_.reset();
  pool_.stop();
  std::string openError;
  bool opened = false;
  int sources = 0;
  if (next.strategy == CaptureStrategy::SystemLoopback) {
    single_ = std::make_unique<CaptureSource>();
    opened = single_->startSystem(openError);
    sources = opened ? 1 : 0;
    if (!opened) {
      code = "AUDIO_INITIALIZATION_FAILED";
    }
  } else if (next.strategy == CaptureStrategy::SingleProcessExclusion) {
    single_ = std::make_unique<CaptureSource>();
    opened = single_->startProcess(next.excludeRootPid, true, openError);
    sources = opened ? 1 : 0;
    if (!opened) {
      code = windowsDocumentsProcessLoopback() ? "PROCESS_CAPTURE_FAILED" : "UNSUPPORTED_WINDOWS";
      if (!windowsDocumentsProcessLoopback()) {
        openError = "ShareGuard requires a newer version of Windows.";
      }
    }
  } else {
    opened = pool_.sync(next.includeRootPids, openError);
    sources = pool_.size();
    if (!opened) {
      code = windowsDocumentsProcessLoopback() ? "PROCESS_CAPTURE_FAILED" : "UNSUPPORTED_WINDOWS";
      if (!windowsDocumentsProcessLoopback()) {
        openError = "ShareGuard requires a newer version of Windows.";
      }
    }
  }

  if (!opened) {
    single_.reset();
    pool_.stop();
    error = openError.empty() ? "Audio capture failed." : openError;
    sourceCount_.store(0);
    return false;
  }

  openedAt_ = now;
  sourceCount_.store(sources);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    plan_ = next;
  }
  Logger::info(std::string("strategy changed: ") + captureStrategyName(next.strategy));
  return true;
}

}
