#include "audio/capture/ProcessCapturePool.hpp"

#include "logging/Logger.hpp"

#include <unordered_set>

namespace shareguard {

bool ProcessCapturePool::dropMissing(const std::vector<std::uint32_t>& rootPids) {
  std::unordered_set<std::uint32_t> desired(rootPids.begin(), rootPids.end());
  bool removed = false;
  for (auto it = sources_.begin(); it != sources_.end();) {
    if (!desired.contains(it->first)) {
      Logger::info("capture source removed pid=" + std::to_string(it->first));
      it = sources_.erase(it);
      removed = true;
    } else {
      ++it;
    }
  }
  return removed;
}

bool ProcessCapturePool::sync(const std::vector<std::uint32_t>& rootPids, std::string& error) {
  dropMissing(rootPids);

  bool anyFailure = false;
  for (const std::uint32_t root : rootPids) {
    const auto existing = sources_.find(root);
    if (existing != sources_.end() && existing->second && existing->second->running()) {
      continue;
    }
    if (existing != sources_.end()) {
      Logger::info("capture source removed pid=" + std::to_string(root));
      sources_.erase(existing);
    }
    auto source = std::make_shared<CaptureSource>();
    std::string sourceError;
    if (!source->startProcess(root, false, sourceError)) {
      Logger::error(sourceError);
      error = sourceError;
      anyFailure = true;
      continue;
    }
    Logger::info("capture source created pid=" + std::to_string(root));
    sources_.emplace(root, std::move(source));
  }
  return !anyFailure || !sources_.empty() || rootPids.empty();
}

bool ProcessCapturePool::matches(const std::vector<std::uint32_t>& rootPids) const {
  if (sources_.size() != rootPids.size()) {
    return false;
  }
  for (const std::uint32_t root : rootPids) {
    const auto it = sources_.find(root);
    if (it == sources_.end() || !it->second || !it->second->running()) {
      return false;
    }
  }
  return true;
}

std::vector<std::shared_ptr<CaptureSource>> ProcessCapturePool::sharedSources() const {
  std::vector<std::shared_ptr<CaptureSource>> output;
  output.reserve(sources_.size());
  for (const auto& entry : sources_) {
    if (entry.second && entry.second->running()) {
      output.push_back(entry.second);
    }
  }
  return output;
}

std::vector<AudioRingBuffer*> ProcessCapturePool::buffers() {
  std::vector<AudioRingBuffer*> output;
  output.reserve(sources_.size());
  for (auto& entry : sources_) {
    if (entry.second && entry.second->running()) {
      output.push_back(&entry.second->buffer());
    }
  }
  return output;
}

std::uint64_t ProcessCapturePool::capturedFrames() const {
  std::uint64_t total = 0;
  for (const auto& entry : sources_) {
    if (entry.second) total += entry.second->capturedFrames();
  }
  return total;
}

std::uint64_t ProcessCapturePool::discontinuities() const {
  std::uint64_t total = 0;
  for (const auto& entry : sources_) {
    if (entry.second) total += entry.second->discontinuities();
  }
  return total;
}

std::uint64_t ProcessCapturePool::droppedFrames() {
  std::uint64_t total = 0;
  for (const auto& entry : sources_) {
    if (entry.second) total += entry.second->buffer().droppedFrames();
  }
  return total;
}

int ProcessCapturePool::sampleRate() const {
  for (const auto& entry : sources_) {
    if (entry.second && entry.second->sampleRate() > 0) return entry.second->sampleRate();
  }
  return 0;
}

void ProcessCapturePool::stop() { sources_.clear(); }

}
