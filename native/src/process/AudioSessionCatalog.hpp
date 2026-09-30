#pragma once

#include "process/ProcessModel.hpp"

#include <cstdint>
#include <unordered_set>

namespace shareguard {

class AudioSessionCatalog {
 public:
  std::unordered_set<std::uint32_t> activeProcessIds() const;
};

}
