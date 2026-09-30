#pragma once

#include "audio/capture/CaptureSource.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace shareguard {

class ProcessCapturePool {
 public:
  bool sync(const std::vector<std::uint32_t>& rootPids, std::string& error);
  bool dropMissing(const std::vector<std::uint32_t>& rootPids);
  bool matches(const std::vector<std::uint32_t>& rootPids) const;
  std::vector<AudioRingBuffer*> buffers();
  std::vector<std::shared_ptr<CaptureSource>> sharedSources() const;
  std::uint64_t capturedFrames() const;
  std::uint64_t discontinuities() const;
  std::uint64_t droppedFrames();
  int sampleRate() const;
  void stop();
  int size() const { return static_cast<int>(sources_.size()); }

 private:
  std::unordered_map<std::uint32_t, std::shared_ptr<CaptureSource>> sources_;
};

}
