#pragma once

#include <atomic>

namespace shareguard {

class ShareSession {
 public:
  void setActive(bool active) { active_.store(active); }
  bool active() const { return active_.load(); }

 private:
  std::atomic<bool> active_{false};
};

}
