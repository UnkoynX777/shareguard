#pragma once

#include "process/ProcessModel.hpp"

#include <string>

namespace shareguard {

struct RuntimeState {
  bool sharing = false;
  bool capturing = false;
  bool protectionEnabled = true;
  CaptureStrategy captureStrategy = CaptureStrategy::SystemLoopback;
  int blockedCount = 0;
  int activeSourceCount = 0;
  std::string lastError;
};

}
