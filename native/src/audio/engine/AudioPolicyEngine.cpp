#include "audio/engine/AudioPolicyEngine.hpp"

#include "process/ProcessTree.hpp"

#include <algorithm>
#include <unordered_set>

namespace shareguard {
namespace {

std::vector<std::uint32_t> allowedIncludeRoots(const ProcessSnapshot& snapshot, const AudioPolicy& policy) {
  std::unordered_set<std::uint32_t> blockedPids;
  for (const ApplicationGroup& group : snapshot.groups) {
    if (!policy.blocks(group.id)) {
      continue;
    }
    blockedPids.insert(group.pids.begin(), group.pids.end());
  }

  std::vector<std::uint32_t> candidates;
  for (const ApplicationGroup& group : snapshot.groups) {
    if (policy.blocks(group.id) || !group.audioActive) {
      continue;
    }
    for (const std::uint32_t root : group.rootPids) {
      candidates.push_back(root);
    }
  }

  std::vector<std::uint32_t> included;
  for (const std::uint32_t root : candidates) {
    bool nested = false;
    for (const std::uint32_t other : candidates) {
      if (other != root && isDescendantOf(root, other, snapshot.parents)) {
        nested = true;
        break;
      }
    }
    if (nested) {
      continue;
    }
    bool leaksBlocked = false;
    for (const std::uint32_t blockedPid : blockedPids) {
      if (isDescendantOf(blockedPid, root, snapshot.parents)) {
        leaksBlocked = true;
        break;
      }
    }
    if (!leaksBlocked) {
      included.push_back(root);
    }
  }
  std::sort(included.begin(), included.end());
  included.erase(std::unique(included.begin(), included.end()), included.end());
  return included;
}

}

CapturePlan AudioPolicyEngine::mixPlan(const ProcessSnapshot& snapshot, const AudioPolicy& policy) const {
  CapturePlan result;
  result.strategy = CaptureStrategy::AllowedProcessMix;
  if (!policy.protectionEnabled) {
    return result;
  }
  result.includeRootPids = allowedIncludeRoots(snapshot, policy);
  return result;
}

CapturePlan AudioPolicyEngine::plan(const ProcessSnapshot& snapshot, const AudioPolicy& policy) const {
  CapturePlan result;
  if (!policy.protectionEnabled) {
    return result;
  }

  std::vector<const ApplicationGroup*> blocked;
  for (const ApplicationGroup& group : snapshot.groups) {
    if (!policy.blocks(group.id)) {
      continue;
    }
    blocked.push_back(&group);
  }
  if (blocked.empty()) {
    return result;
  }

  int blockedRoots = 0;
  for (const ApplicationGroup* group : blocked) {
    blockedRoots += static_cast<int>(group->rootPids.size());
  }
  if (blocked.size() == 1 && blockedRoots == 1 && !blocked.front()->rootPids.empty()) {
    result.strategy = CaptureStrategy::SingleProcessExclusion;
    result.excludeRootPid = blocked.front()->rootPids.front();
    return result;
  }
  return mixPlan(snapshot, policy);
}

CapturePlan AudioPolicyEngine::sessionPlan(const ProcessSnapshot& snapshot, const AudioPolicy& policy,
                                           CaptureStrategy floor) const {
  if (!policy.protectionEnabled) {
    return {};
  }
  const CapturePlan raw = plan(snapshot, policy);
  if (raw.strategy == CaptureStrategy::SingleProcessExclusion && floor == CaptureStrategy::AllowedProcessMix) {
    return mixPlan(snapshot, policy);
  }
  return raw;
}

}
