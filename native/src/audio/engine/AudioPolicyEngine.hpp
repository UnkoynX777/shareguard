#pragma once

#include "policy/AudioPolicy.hpp"
#include "process/ProcessModel.hpp"

namespace shareguard {

class AudioPolicyEngine {
 public:
  CapturePlan plan(const ProcessSnapshot& snapshot, const AudioPolicy& policy) const;
};

}
