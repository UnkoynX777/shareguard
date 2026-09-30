#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace shareguard {

struct ProcessInfo {
  std::uint32_t pid = 0;
  std::uint32_t parentPid = 0;
  std::uint32_t rootPid = 0;
  std::wstring executableName;
  std::wstring executablePath;
  std::string displayName;
  bool audioActive = false;
  bool hasWindow = false;
};

struct ApplicationGroup {
  std::string id;
  std::string displayName;
  std::string executableName;
  std::vector<std::uint32_t> rootPids;
  std::vector<std::uint32_t> pids;
  bool audioActive = false;
  bool hasWindow = false;
  bool interactive = false;
};

struct ProcessSnapshot {
  std::vector<ApplicationGroup> groups;
  std::unordered_map<std::uint32_t, std::uint32_t> parents;
};

enum class CaptureStrategy { SystemLoopback, SingleProcessExclusion, AllowedProcessMix };

struct CapturePlan {
  CaptureStrategy strategy = CaptureStrategy::SystemLoopback;
  std::uint32_t excludeRootPid = 0;
  std::vector<std::uint32_t> includeRootPids;

  bool operator==(const CapturePlan& other) const {
    return strategy == other.strategy && excludeRootPid == other.excludeRootPid &&
           includeRootPids == other.includeRootPids;
  }
};

const char* captureStrategyName(CaptureStrategy strategy);

}
