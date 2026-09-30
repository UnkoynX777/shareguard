#include "audio/engine/AudioEngine.hpp"

#include "logging/Logger.hpp"
#include "platform/windows/ComRuntime.hpp"
#include "platform/windows/WindowsVersion.hpp"

#include <algorithm>
#include <cstring>

namespace shareguard {
namespace {

std::string sessionText(std::uint64_t session) { return "session=" + std::to_string(session); }

}

AudioEngine::~AudioEngine() { stop(); }

bool AudioEngine::start(const AudioPolicy& policy, SnapshotHandler snapshot, FrameHandler onFrame, ErrorHandler onError,
                        std::string& error) {
  stop();
  failureCode_.clear();
  sessionId_.fetch_add(1);
  generation_.store(0);
  appliedRevision_.store(0);
  notifiedRevision_ = 0;
  strategyFloor_ = CaptureStrategy::SystemLoopback;
  sequence_ = 0;
  incremental_.store(0);
  fullRebuilds_.store(0);
  expectedStops_.store(0);
  unexpectedStops_.store(0);
  staleIgnored_.store(0);
  mixedFrames_.store(0);
  sentFrames_.store(0);
  transportDrops_.store(0);
  starved_.store(0);
  queueFrames_.store(0);
  live_.store(false);
  snapshot_ = std::move(snapshot);
  onFrame_ = std::move(onFrame);
  onError_ = std::move(onError);
  plan_ = {};
  mailbox_.reset();
  mailbox_.push(policy);
  {
    std::lock_guard<std::mutex> lock(startMutex_);
    startFinished_ = false;
    startOk_ = false;
    startError_.clear();
  }
  dirty_.store(true);
  capturing_.store(true);
  writer_ = std::jthread([this](std::stop_token stopToken) { writeLoop(stopToken); });
  thread_ = std::jthread([this](std::stop_token stopToken) { loop(stopToken); });
  control_ = std::jthread([this](std::stop_token stopToken) { controlLoop(stopToken); });
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
  if (control_.joinable()) {
    control_.request_stop();
    controlCv_.notify_all();
    control_.join();
  }
  live_.store(false);
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
  published_.store(nullptr);
  retired_.clear();
  direct_.reset();
  pool_.stop();
  sourceCount_.store(0);
  mailbox_.reset();
  strategyFloor_ = CaptureStrategy::SystemLoopback;
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

std::uint64_t AudioEngine::applyPolicy(const AudioPolicy& policy) {
  const std::uint64_t revision = mailbox_.push(policy);
  dirty_.store(true);
  controlCv_.notify_all();
  return revision;
}

void AudioEngine::noteSnapshot() {
  dirty_.store(true);
  controlCv_.notify_all();
}

void AudioEngine::setAppliedHandler(AppliedHandler handler) { onApplied_ = std::move(handler); }

CaptureStrategy AudioEngine::strategy() const {
  if (const auto graph = published_.load()) {
    return graph->strategy;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  return plan_.strategy;
}

void AudioEngine::markApplied(std::uint64_t revision, CaptureStrategy strategy, int blockedCount) {
  appliedRevision_.store(revision);
  if (revision == 0 || revision == notifiedRevision_) {
    return;
  }
  notifiedRevision_ = revision;
  if (onApplied_) onApplied_(revision, strategy, blockedCount);
}

std::shared_ptr<AudioGraph> AudioEngine::makeGraph(CaptureStrategy strategy,
                                                   std::vector<std::shared_ptr<CaptureSource>> sources) const {
  auto graph = std::make_shared<AudioGraph>();
  graph->strategy = strategy;
  graph->sources = std::move(sources);
  graph->inputs.reserve(graph->sources.size());
  for (const auto& source : graph->sources) {
    if (source && source->running()) {
      graph->inputs.push_back(&source->buffer());
    }
  }
  return graph;
}

void AudioEngine::reap() {
  std::erase_if(retired_, [](const std::shared_ptr<AudioGraph>& graph) { return graph.use_count() == 1; });
}

void AudioEngine::publish(const std::shared_ptr<AudioGraph>& graph, const char* reason) {
  if (graph->generation == 0) {
    graph->generation = generation_.fetch_add(1) + 1;
  }
  const auto previous = published_.exchange(graph);
  const std::uint64_t previousGeneration = previous ? previous->generation : 0;
  if (previous) {
    retired_.push_back(previous);
    expectedStops_.fetch_add(1);
  }
  sourceCount_.store(static_cast<int>(graph->sources.size()));
  live_.store(true);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    plan_.strategy = graph->strategy;
    plan_.excludeRootPid = 0;
    plan_.includeRootPids.clear();
  }
  Logger::info(sessionText(sessionId_.load()) + " graph swap " + std::to_string(previousGeneration) + " -> " +
               std::to_string(graph->generation) + " strategy=" + captureStrategyName(graph->strategy) +
               " reason=" + reason + " sources=" + std::to_string(graph->sources.size()));
  reap();
}

bool AudioEngine::openDirect(bool exclude, std::uint32_t pid, std::string& error) {
  auto source = std::make_shared<CaptureSource>();
  const bool opened = exclude ? source->startProcess(pid, true, error) : source->startSystem(error);
  if (!opened) {
    if (exclude && !windowsDocumentsProcessLoopback()) {
      error = "ShareGuard requires a newer version of Windows.";
    }
    return false;
  }
  direct_ = std::move(source);
  return true;
}

bool AudioEngine::openMix(const std::vector<std::uint32_t>& rootPids, std::string& error) {
  const bool opened = pool_.sync(rootPids, error);
  if (!opened && pool_.size() == 0 && !rootPids.empty()) {
    if (!windowsDocumentsProcessLoopback()) {
      error = "ShareGuard requires a newer version of Windows.";
    }
    return false;
  }
  return true;
}

void AudioEngine::checkHealth() {
  const auto graph = published_.load();
  if (!graph || graph->sources.empty()) {
    return;
  }
  for (const auto& source : graph->sources) {
    if (source && !source->running()) {
      const std::uint64_t count = unexpectedStops_.fetch_add(1) + 1;
      Logger::error(sessionText(sessionId_.load()) + " generation=" + std::to_string(graph->generation) +
                    " strategy=" + captureStrategyName(graph->strategy) +
                    " reason=UnexpectedTermination");
      if (count >= 5 && capturing_.load()) {
        failureCode_ = "CAPTURE_UNEXPECTED_TERMINATION";
        if (onError_) onError_(failureCode_, "Capture stopped unexpectedly.");
        capturing_.store(false);
      } else {
        dirty_.store(true);
      }
      return;
    }
  }
  unexpectedStops_.store(0);
}

bool AudioEngine::reconcile(std::stop_token stop, std::string& error) {
  AudioPolicy policy;
  const std::uint64_t seen = mailbox_.sample(policy);
  if (stop.stop_requested()) {
    return true;
  }
  const ProcessSnapshot snapshot = snapshot_ ? snapshot_() : ProcessSnapshot{};
  CapturePlan next = policyEngine_.sessionPlan(snapshot, policy, strategyFloor_);
  if (next.strategy == CaptureStrategy::AllowedProcessMix) {
    strategyFloor_ = CaptureStrategy::AllowedProcessMix;
  } else if (next.strategy == CaptureStrategy::SystemLoopback) {
    strategyFloor_ = CaptureStrategy::SystemLoopback;
  }
  if (mailbox_.revision() != seen) {
    staleIgnored_.fetch_add(1);
    dirty_.store(true);
    Logger::info(sessionText(sessionId_.load()) + " policy=" + std::to_string(seen) + " stale revision ignored");
    return true;
  }

  const auto current = published_.load();
  const bool sameStrategy = current && current->strategy == next.strategy;
  if (sameStrategy && next.strategy == CaptureStrategy::SystemLoopback && direct_ && direct_->running() &&
      current->sources.size() == 1) {
    markApplied(seen, next.strategy, static_cast<int>(policy.blockedIds.size()));
    return true;
  }
  if (sameStrategy && next.strategy == CaptureStrategy::SingleProcessExclusion && direct_ && direct_->running() &&
      current->excludeRootPid == next.excludeRootPid && current->sources.size() == 1) {
    markApplied(seen, next.strategy, static_cast<int>(policy.blockedIds.size()));
    return true;
  }
  if (sameStrategy && next.strategy == CaptureStrategy::AllowedProcessMix && pool_.matches(next.includeRootPids)) {
    markApplied(seen, next.strategy, static_cast<int>(policy.blockedIds.size()));
    return true;
  }

  const bool restrictive = current && next.strategy == CaptureStrategy::AllowedProcessMix &&
                           current->strategy != CaptureStrategy::AllowedProcessMix;
  if (restrictive) {
    publish(makeGraph(next.strategy, {}), "PolicyReconfiguration");
    direct_.reset();
    Logger::info(sessionText(sessionId_.load()) + " policy=" + std::to_string(seen) +
                 " silence bridge reason=PolicyReconfiguration");
  }

  std::string openError;
  bool opened = false;
  if (next.strategy == CaptureStrategy::AllowedProcessMix && current &&
      current->strategy == CaptureStrategy::AllowedProcessMix && pool_.dropMissing(next.includeRootPids)) {
    incremental_.fetch_add(1);
    publish(makeGraph(CaptureStrategy::AllowedProcessMix, pool_.sharedSources()), "PolicyReconfiguration");
  }

  const char* reason = "blocked policy spans multiple process trees";
  if (next.strategy == CaptureStrategy::SystemLoopback) {
    reason = "nothing blocked";
  } else if (next.strategy == CaptureStrategy::SingleProcessExclusion) {
    reason = "single blockable process tree";
  } else if (policyEngine_.plan(snapshot, policy).strategy == CaptureStrategy::SingleProcessExclusion) {
    reason = "keep AllowedProcessMix for this session";
  }
  const std::uint64_t candidate = generation_.fetch_add(1) + 1;
  Logger::info(sessionText(sessionId_.load()) + " policy=" + std::to_string(seen) + " client=" +
               std::to_string(policy.clientRevision) + " generation=" + std::to_string(candidate) + " prepare strategy=" +
               captureStrategyName(next.strategy) + " reason=" + reason + " target=" +
               std::to_string(next.excludeRootPid) + " sources=" + std::to_string(next.includeRootPids.size()));

  if (next.strategy == CaptureStrategy::SystemLoopback) {
    opened = openDirect(false, 0, openError);
  } else if (next.strategy == CaptureStrategy::SingleProcessExclusion) {
    pool_.stop();
    opened = openDirect(true, next.excludeRootPid, openError);
  } else {
    direct_.reset();
    opened = openMix(next.includeRootPids, openError);
    if (!opened && pool_.sharedSources().empty() && !live_.load()) {
      if (stop.stop_requested() || mailbox_.revision() != seen) {
        staleIgnored_.fetch_add(1);
        dirty_.store(true);
        Logger::info(sessionText(sessionId_.load()) + " generation=" + std::to_string(candidate) +
                     " activation completed stale generation ignored");
        return true;
      }
      error = openError.empty() ? "Audio capture failed." : openError;
      Logger::error(sessionText(sessionId_.load()) + " generation=" + std::to_string(candidate) + " strategy=" +
                    captureStrategyName(next.strategy) + " open failed " + error);
      failureCode_ = windowsDocumentsProcessLoopback() ? "PROCESS_CAPTURE_FAILED" : "UNSUPPORTED_WINDOWS";
      return false;
    }
    if (!opened) {
      Logger::error(sessionText(sessionId_.load()) + " generation=" + std::to_string(candidate) +
                    " strategy=" + captureStrategyName(next.strategy) + " source open failed " +
                    (openError.empty() ? "Audio capture failed." : openError));
    }
    opened = true;
  }

  if (stop.stop_requested() || mailbox_.revision() != seen) {
    staleIgnored_.fetch_add(1);
    dirty_.store(true);
    if (next.strategy != CaptureStrategy::AllowedProcessMix) {
      direct_.reset();
    }
    Logger::info(sessionText(sessionId_.load()) + " generation=" + std::to_string(candidate) +
                 " activation completed stale generation ignored");
    return true;
  }

  if (!opened) {
    error = openError.empty() ? "Audio capture failed." : openError;
    Logger::error(sessionText(sessionId_.load()) + " generation=" + std::to_string(candidate) + " strategy=" +
                  captureStrategyName(next.strategy) + " open failed " + error);
    if (!live_.load()) {
      failureCode_ = next.strategy == CaptureStrategy::SystemLoopback ? "AUDIO_INITIALIZATION_FAILED"
                                                                       : (windowsDocumentsProcessLoopback() ? "PROCESS_CAPTURE_FAILED"
                                                                                                             : "UNSUPPORTED_WINDOWS");
      return false;
    }
    return true;
  }

  std::vector<std::shared_ptr<CaptureSource>> sources;
  if (next.strategy == CaptureStrategy::AllowedProcessMix) {
    sources = pool_.sharedSources();
  } else if (direct_) {
    sources.push_back(direct_);
  }
  auto graph = makeGraph(next.strategy, std::move(sources));
  graph->generation = candidate;
  graph->excludeRootPid = next.excludeRootPid;
  const bool incremental = current && current->strategy == CaptureStrategy::AllowedProcessMix &&
                           next.strategy == CaptureStrategy::AllowedProcessMix;
  if (incremental) {
    incremental_.fetch_add(1);
  } else {
    fullRebuilds_.fetch_add(1);
  }
  if (next.strategy == CaptureStrategy::SystemLoopback) {
    pool_.stop();
  }
  publish(graph, incremental ? "PolicyReconfiguration" : "StrategyReplacement");
  Logger::info(sessionText(sessionId_.load()) + " policy=" + std::to_string(seen) + " applied strategy=" +
               captureStrategyName(next.strategy) + " generation=" + std::to_string(candidate));
  markApplied(seen, next.strategy, static_cast<int>(policy.blockedIds.size()));
  return true;
}

void AudioEngine::controlLoop(std::stop_token stop) {
  ComRuntime com;
  bool announced = false;
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
  if (!com.ok()) {
    announce(false, "COM initialization failed.");
    return;
  }

  while (!stop.stop_requested()) {
    {
      std::unique_lock<std::mutex> lock(controlMutex_);
      controlCv_.wait_for(lock, stop, std::chrono::seconds(1), [&] { return dirty_.load() || stop.stop_requested(); });
    }
    if (stop.stop_requested()) {
      break;
    }
    reap();
    if (dirty_.exchange(false) || !live_.load()) {
      std::string error;
      if (!reconcile(stop, error)) {
        capturing_.store(false);
        if (failureCode_.empty()) failureCode_ = "AUDIO_INITIALIZATION_FAILED";
        if (announced && onError_) onError_(failureCode_, error);
        announce(false, error);
        break;
      }
      if (live_.load()) announce(true, {});
    } else {
      checkHealth();
    }
  }
  announce(false, "Capture stopped.");
}

void AudioEngine::logDiagnostics(std::chrono::steady_clock::time_point& lastLog, std::uint64_t& lastCaptured,
                                 std::uint64_t& lastMixed, std::uint64_t& lastSent) {
  const auto now = std::chrono::steady_clock::now();
  if (now - lastLog < std::chrono::seconds(2)) {
    return;
  }
  const double seconds = std::chrono::duration<double>(now - lastLog).count();
  lastLog = now;
  const auto graph = published_.load();
  std::uint64_t captured = 0;
  std::uint64_t breaks = 0;
  std::uint64_t drops = transportDrops_.load();
  int rate = 0;
  size_t queued = static_cast<size_t>(std::max(0, queueFrames_.load()));
  if (graph) {
    for (const auto& source : graph->sources) {
      if (!source) continue;
      captured += source->capturedFrames();
      breaks += source->discontinuities();
      drops += source->buffer().droppedFrames();
      if (rate == 0) rate = source->sampleRate();
      queued += source->buffer().available();
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
               " waits=" + std::to_string(starved_.load()) + " rate=" + std::to_string(rate) +
               " session=" + std::to_string(sessionId_.load()) + " generation=" + std::to_string(generation_.load()) +
               " revision=" + std::to_string(appliedRevision_.load()) + "/" + std::to_string(mailbox_.revision()) +
               " received=" + std::to_string(mailbox_.received()) + " coalesced=" + std::to_string(mailbox_.coalesced()) +
               " incremental=" + std::to_string(incremental_.load()) + " rebuilds=" + std::to_string(fullRebuilds_.load()) +
               " expectedStops=" + std::to_string(expectedStops_.load()) +
               " unexpected=" + std::to_string(unexpectedStops_.load()) + " stale=" + std::to_string(staleIgnored_.load()) +
               " depth=" + std::to_string(mailbox_.depth()));
}

void AudioEngine::loop(std::stop_token stop) {
  auto lastLog = std::chrono::steady_clock::now();
  std::uint64_t lastCaptured = 0;
  std::uint64_t lastMixed = 0;
  std::uint64_t lastSent = 0;

  while (!stop.stop_requested()) {
    const auto graph = published_.load();
    if (!live_.load() || !graph) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      logDiagnostics(lastLog, lastCaptured, lastMixed, lastSent);
      continue;
    }
    if (graph->inputs.empty()) {
      float block[kPacketFrames * 2] = {};
      AudioFrame frame;
      frame.sequence = ++sequence_;
      quantizeStereo(block, kPacketFrames, frame.pcm);
      enqueue(std::move(frame));
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      logDiagnostics(lastLog, lastCaptured, lastMixed, lastSent);
      continue;
    }

    std::vector<AudioRingBuffer*> ready;
    AudioRingBuffer* waiting = nullptr;
    size_t least = static_cast<size_t>(-1);
    int inputs = 0;
    ready.reserve(graph->inputs.size());
    for (AudioRingBuffer* input : graph->inputs) {
      if (input == nullptr) continue;
      ++inputs;
      const size_t have = input->available();
      if (have >= static_cast<size_t>(kPacketFrames)) {
        ready.push_back(input);
      } else if (have < least) {
        least = have;
        waiting = input;
      }
    }
    const bool complete = inputs > 0 && static_cast<int>(ready.size()) == inputs;
    if (!complete && ready.empty()) {
      starved_.fetch_add(1);
      if (waiting != nullptr) {
        waiting->waitFor(static_cast<size_t>(kPacketFrames), stop, std::chrono::milliseconds(10));
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    } else if (!complete) {
      starved_.fetch_add(1);
      if (waiting != nullptr) {
        waiting->waitFor(static_cast<size_t>(kPacketFrames), stop, std::chrono::milliseconds(20));
      }
      ready.clear();
      for (AudioRingBuffer* input : graph->inputs) {
        if (input != nullptr && input->available() >= static_cast<size_t>(kPacketFrames)) {
          ready.push_back(input);
        }
      }
      if (ready.empty()) {
        continue;
      }
      float block[kPacketFrames * 2];
      mixer_.mix(ready, block, kPacketFrames);
      AudioFrame frame;
      frame.sequence = ++sequence_;
      quantizeStereo(block, kPacketFrames, frame.pcm);
      enqueue(std::move(frame));
    } else {
      float block[kPacketFrames * 2];
      mixer_.mix(ready, block, kPacketFrames);
      AudioFrame frame;
      frame.sequence = ++sequence_;
      quantizeStereo(block, kPacketFrames, frame.pcm);
      enqueue(std::move(frame));
    }
    logDiagnostics(lastLog, lastCaptured, lastMixed, lastSent);
  }
}

}
