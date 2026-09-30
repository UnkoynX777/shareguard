#pragma once

#include "policy/AudioPolicy.hpp"
#include "process/ProcessModel.hpp"

namespace shareguard {

class AudioPolicyEngine {
 public:
  CapturePlan plan(const ProcessSnapshot& snapshot, const AudioPolicy& policy) const;
  CapturePlan sessionPlan(const ProcessSnapshot& snapshot, const AudioPolicy& policy, CaptureStrategy floor) const;

 private:
  CapturePlan mixPlan(const ProcessSnapshot& snapshot, const AudioPolicy& policy) const;
};

}
