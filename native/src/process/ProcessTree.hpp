#pragma once

#include "process/ProcessModel.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace shareguard {

struct RawProcess {
  std::uint32_t pid = 0;
  std::uint32_t parentPid = 0;
  std::wstring executableName;
  std::wstring executablePath;
};

std::vector<ApplicationGroup> groupProcesses(const std::vector<RawProcess>& processes,
                                             const std::unordered_set<std::uint32_t>& audioPids,
                                             const std::unordered_set<std::uint32_t>& windowPids,
                                             bool includeBackground);

bool isDescendantOf(std::uint32_t pid, std::uint32_t ancestor,
                    const std::unordered_map<std::uint32_t, std::uint32_t>& parents);

}
