#pragma once

#include "messaging/NativeMessagingHost.hpp"
#include "policy/AudioPolicy.hpp"
#include "process/ProcessModel.hpp"

#include <functional>
#include <string>

namespace shareguard {

class MessageDispatcher {
 public:
  struct Actions {
    std::function<ProcessSnapshot(bool includeBackground)> snapshot;
    std::function<void(bool includeBackground)> setIncludeBackground;
    std::function<void(const AudioPolicy& policy)> applyPolicy;
    std::function<bool(std::string& code, std::string& error)> startCapture;
    std::function<void()> stopCapture;
    std::function<std::string()> status;
    std::function<const char*()> strategyName;
    std::function<int()> blockedCount;
  };

  void handle(const std::string& json, const std::function<void(const std::string&)>& send, Actions& actions);
};

}
