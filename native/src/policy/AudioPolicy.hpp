#pragma once

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

  bool blocks(const std::string& identity) const;
};

}
