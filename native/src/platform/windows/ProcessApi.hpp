#pragma once

#include "process/ProcessTree.hpp"

#include <cstdint>
#include <unordered_set>
#include <vector>

namespace shareguard {

std::vector<RawProcess> enumerateProcesses();
std::unordered_set<std::uint32_t> visibleWindowProcessIds();
std::unordered_set<std::uint32_t> activeAudioProcessIds();

}
