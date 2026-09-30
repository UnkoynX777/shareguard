#include "policy/AudioPolicy.hpp"

#include <algorithm>

namespace shareguard {

bool AudioPolicy::blocks(const std::string& identity) const {
  if (!protectionEnabled) {
    return false;
  }
  return std::find(blockedIds.begin(), blockedIds.end(), identity) != blockedIds.end();
}

}
