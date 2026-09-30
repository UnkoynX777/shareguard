#pragma once

#include "policy/AudioPolicy.hpp"

#include <cstdint>
#include <mutex>

namespace shareguard {

class PolicyMailbox {
 public:
  std::uint64_t push(const AudioPolicy& policy) {
    std::lock_guard<std::mutex> lock(mutex_);
    ++received_;
    if (policy.clientRevision != 0 && policy.clientRevision < revision_) {
      return revision_;
    }
    policy_ = policy;
    if (policy.clientRevision > revision_) {
      revision_ = policy.clientRevision;
    } else {
      ++revision_;
    }
    if (pending_) {
      ++coalesced_;
    }
    pending_ = true;
    return revision_;
  }

  std::uint64_t sample(AudioPolicy& policy) {
    std::lock_guard<std::mutex> lock(mutex_);
    policy = policy_;
    pending_ = false;
    return revision_;
  }

  std::uint64_t revision() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return revision_;
  }

  std::uint64_t received() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return received_;
  }

  std::uint64_t coalesced() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return coalesced_;
  }

  int depth() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pending_ ? 1 : 0;
  }

  void reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    policy_ = {};
    revision_ = 0;
    received_ = 0;
    coalesced_ = 0;
    pending_ = false;
  }

 private:
  mutable std::mutex mutex_;
  AudioPolicy policy_{};
  std::uint64_t revision_ = 0;
  std::uint64_t received_ = 0;
  std::uint64_t coalesced_ = 0;
  bool pending_ = false;
};

}
