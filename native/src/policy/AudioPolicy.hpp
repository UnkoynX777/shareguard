#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace shareguard {

struct ProcessRule {
  std::string identity;
  bool blocked = false;
};

struct AudioPolicy {
  bool protectionEnabled = true;
  std::vector<std::string> blockedIds;
  std::uint64_t clientRevision = 0;

  bool blocks(const std::string& identity) const;
};

}
