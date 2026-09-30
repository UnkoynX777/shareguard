#include "audio/engine/AudioEngine.hpp"

#include "logging/Logger.hpp"
#include "platform/windows/ComRuntime.hpp"
#include "platform/windows/WindowsVersion.hpp"

#include <algorithm>
#include <cstring>
#include <deque>

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
  mixedFrames_.store(0);
  sentFrames_.store(0);
  transportDrops_.store(0);
  queueFrames_.store(0);
  writer_ = std::jthread([this](std::stop_token stop) { writeLoop(stop); });
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
  if (writer_.joinable()) {
    writer_.request_stop();
    queueReady_.notify_all();
    writer_.join();
  }
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.clear();
  }
  queueFrames_.store(0);
  single_.reset();
  pool_.stop();
  sourceCount_.store(0);
}

void AudioEngine::enqueue(AudioFrame frame) {
  const int count = frame.frameCount;
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    constexpr size_t kQueueLimit = 12;
    while (queue_.size() >= kQueueLimit) {
      transportDrops_.fetch_add(static_cast<std::uint64_t>(queue_.front().frameCount));
      queue_.pop_front();
    }
    queue_.push_back(std::move(frame));
    int queued = 0;
    for (const AudioFrame& item : queue_) queued += item.frameCount;
    queueFrames_.store(queued);
  }
  queueReady_.notify_one();
  mixedFrames_.fetch_add(static_cast<std::uint64_t>(count));
}

void AudioEngine::writeLoop(std::stop_token stop) {
  while (!stop.stop_requested()) {
    AudioFrame frame;
    {
      std::unique_lock<std::mutex> lock(queueMutex_);
      queueReady_.wait(lock, stop, [&] { return !queue_.empty(); });
      if (queue_.empty()) {
        continue;
      }
      frame = std::move(queue_.front());
      queue_.pop_front();
      int queued = 0;
      for (const AudioFrame& item : queue_) queued += item.frameCount;
      queueFrames_.store(queued);
    }
    sentFrames_.fetch_add(static_cast<std::uint64_t>(frame.frameCount));
    if (onFrame_) onFrame_(frame);
  }
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

void AudioEngine::logDiagnostics(std::chrono::steady_clock::time_point& lastLog, std::uint64_t& lastCaptured,
                                  std::uint64_t& lastMixed, std::uint64_t& lastSent) {
  const auto now = std::chrono::steady_clock::now();
  if (now - lastLog < std::chrono::seconds(2)) {
    return;
  }
  const double seconds = std::chrono::duration<double>(now - lastLog).count();
  lastLog = now;
  std::uint64_t captured = 0;
  std::uint64_t breaks = 0;
  std::uint64_t drops = transportDrops_.load();
  int rate = 0;
  size_t queued = static_cast<size_t>(std::max(0, queueFrames_.load()));
  if (single_) {
    captured = single_->capturedFrames();
    breaks = single_->discontinuities();
    drops += single_->buffer().droppedFrames();
    rate = single_->sampleRate();
    queued += single_->buffer().available();
  } else {
    captured = pool_.capturedFrames();
    breaks = pool_.discontinuities();
    drops += pool_.droppedFrames();
    rate = pool_.sampleRate();
    for (AudioRingBuffer* input : pool_.buffers()) {
      if (input) queued += input->available();
    }
  }
  const auto perSecond = [&](std::uint64_t current, std::uint64_t& previous) {
    const std::uint64_t delta = current >= previous ? current - previous : 0;
    previous = current;
    return seconds > 0.0 ? static_cast<int>(static_cast<double>(delta) / seconds) : 0;
  };
  const int captureRate = perSecond(captured, lastCaptured);
  const std::uint64_t mixed = mixedFrames_.load();
  const int mixedRate = perSecond(mixed, lastMixed);
  const int sentRate = perSecond(sentFrames_.load(), lastSent);
  const int queueMs = static_cast<int>((queued * 1000) / kCanonicalRate);
  Logger::info("audio capture=" + std::to_string(captureRate) + "/s mixed=" + std::to_string(mixedRate) +
               "/s sent=" + std::to_string(sentRate) + "/s queue=" + std::to_string(queueMs) +
               "ms discontinuities=" + std::to_string(breaks) + " drops=" + std::to_string(drops) +
               " waits=" + std::to_string(starved_.load()) + " rate=" + std::to_string(rate));
}

void AudioEngine::loop(std::stop_token stop) {
  ComRuntime com;
  bool announced = false;
  auto lastLog = std::chrono::steady_clock::now();
  std::uint64_t lastCaptured = 0;
  std::uint64_t lastMixed = 0;
  std::uint64_t lastSent = 0;
  auto lastRebuildCheck = lastLog;
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

    std::vector<AudioRingBuffer*> inputs;
    if (single_ && single_->running()) {
      inputs.push_back(&single_->buffer());
    } else {
      inputs = pool_.buffers();
    }
    AudioRingBuffer* fullest = nullptr;
    size_t ready = 0;
    for (AudioRingBuffer* input : inputs) {
      if (input == nullptr) continue;
      const size_t have = input->available();
      if (have >= ready) {
        ready = have;
        fullest = input;
      }
    }
    if (ready < static_cast<size_t>(kPacketFrames)) {
      starved_.fetch_add(1);
      if (fullest != nullptr) {
        fullest->waitFor(static_cast<size_t>(kPacketFrames), stop, std::chrono::milliseconds(10));
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    } else {
      float block[kPacketFrames * 2];
      mixer_.mix(inputs, block, kPacketFrames);
      AudioFrame frame;
      frame.sequence = ++sequence_;
      quantizeStereo(block, kPacketFrames, frame.pcm);
      enqueue(std::move(frame));
    }
    logDiagnostics(lastLog, lastCaptured, lastMixed, lastSent);
    const auto now = std::chrono::steady_clock::now();
    if (now - lastRebuildCheck >= std::chrono::seconds(1) ||
        (single_ && !single_->running() && plan_.strategy != CaptureStrategy::AllowedProcessMix)) {
      lastRebuildCheck = std::chrono::steady_clock::now();
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
