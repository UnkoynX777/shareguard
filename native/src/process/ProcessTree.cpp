#include "process/ProcessTree.hpp"

#include "platform/windows/WinHandle.hpp"
#include "process/ProcessIdentity.hpp"

#include <algorithm>
#include <cwctype>

namespace shareguard {
namespace {

bool sameExecutable(const std::wstring& left, const std::wstring& right) {
  return toLowerWide(left) == toLowerWide(right);
}

std::uint32_t findRoot(std::uint32_t pid, const std::unordered_map<std::uint32_t, const RawProcess*>& byPid) {
  const auto start = byPid.find(pid);
  if (start == byPid.end()) {
    return pid;
  }
  std::uint32_t current = pid;
  const std::wstring executable = start->second->executableName;
  for (int depth = 0; depth < 32; ++depth) {
    const auto node = byPid.find(current);
    if (node == byPid.end()) {
      break;
    }
    const auto parent = byPid.find(node->second->parentPid);
    if (parent == byPid.end() || !sameExecutable(parent->second->executableName, executable)) {
      return current;
    }
    current = parent->first;
  }
  return current;
}

}

bool isDescendantOf(std::uint32_t pid, std::uint32_t ancestor,
                    const std::unordered_map<std::uint32_t, std::uint32_t>& parents) {
  std::uint32_t current = pid;
  for (int depth = 0; depth < 32; ++depth) {
    const auto parent = parents.find(current);
    if (parent == parents.end() || parent->second == 0 || parent->second == current) {
      return false;
    }
    if (parent->second == ancestor) {
      return true;
    }
    current = parent->second;
  }
  return false;
}

std::vector<ApplicationGroup> groupProcesses(const std::vector<RawProcess>& processes,
                                             const std::unordered_set<std::uint32_t>& audioPids,
                                             const std::unordered_set<std::uint32_t>& windowPids,
                                             bool includeBackground) {
  std::unordered_map<std::uint32_t, const RawProcess*> byPid;
  byPid.reserve(processes.size());
  for (const RawProcess& process : processes) {
    if (process.pid != 0 && !process.executableName.empty()) {
      byPid.emplace(process.pid, &process);
    }
  }

  std::unordered_map<std::string, ApplicationGroup> grouped;
  for (const RawProcess& process : processes) {
    if (process.pid == 0 || process.executableName.empty()) {
      continue;
    }
    const std::uint32_t root = findRoot(process.pid, byPid);
    const std::string id = processIdentity(process.executableName);
    ApplicationGroup& group = grouped[id];
    if (group.id.empty()) {
      group.id = id;
      group.executableName = wideToUtf8(process.executableName);
    }
    group.pids.push_back(process.pid);
    if (std::find(group.rootPids.begin(), group.rootPids.end(), root) == group.rootPids.end()) {
      group.rootPids.push_back(root);
    }
    if (audioPids.contains(process.pid)) {
      group.audioActive = true;
    }
    if (windowPids.contains(process.pid)) {
      group.hasWindow = true;
    }
    if (root == process.pid && group.displayName.empty()) {
      group.displayName = displayNameForPath(process.executablePath, process.executableName);
    }
  }

  std::vector<ApplicationGroup> result;
  result.reserve(grouped.size());
  for (auto& entry : grouped) {
    ApplicationGroup& group = entry.second;
    group.interactive = group.audioActive || group.hasWindow;
    if (!includeBackground && !group.interactive) {
      continue;
    }
    if (group.displayName.empty()) {
      group.displayName = group.executableName;
    }
    std::sort(group.rootPids.begin(), group.rootPids.end());
    std::sort(group.pids.begin(), group.pids.end());
    result.push_back(std::move(group));
  }
  std::sort(result.begin(), result.end(), [](const ApplicationGroup& left, const ApplicationGroup& right) {
    if (left.audioActive != right.audioActive) {
      return left.audioActive;
    }
    return left.displayName < right.displayName;
  });
  return result;
}

}
