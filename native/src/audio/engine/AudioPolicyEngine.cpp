#include "audio/engine/AudioPolicyEngine.hpp"

#include "process/ProcessTree.hpp"

#include <algorithm>
#include <unordered_set>

namespace shareguard {

CapturePlan AudioPolicyEngine::plan(const ProcessSnapshot& snapshot, const AudioPolicy& policy) const {
  CapturePlan result;
  if (!policy.protectionEnabled) {
    return result;
  }

  std::vector<const ApplicationGroup*> blocked;
  std::unordered_set<std::uint32_t> blockedPids;
  for (const ApplicationGroup& group : snapshot.groups) {
    if (!policy.blocks(group.id)) {
      continue;
    }
    blocked.push_back(&group);
    blockedPids.insert(group.pids.begin(), group.pids.end());
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

  result.strategy = CaptureStrategy::AllowedProcessMix;
  std::vector<std::uint32_t> candidates;
  for (const ApplicationGroup& group : snapshot.groups) {
    if (policy.blocks(group.id) || !group.audioActive) {
      continue;
    }
    for (const std::uint32_t root : group.rootPids) {
      candidates.push_back(root);
    }
  }

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
      result.includeRootPids.push_back(root);
    }
  }
  std::sort(result.includeRootPids.begin(), result.includeRootPids.end());
  result.includeRootPids.erase(std::unique(result.includeRootPids.begin(), result.includeRootPids.end()),
                               result.includeRootPids.end());
  return result;
}

}
