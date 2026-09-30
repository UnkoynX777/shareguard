#include "messaging/MessageDispatcher.hpp"

#include "logging/Logger.hpp"
#include "messaging/Protocol.hpp"
#include "process/ProcessModel.hpp"

namespace shareguard {

void MessageDispatcher::handle(const std::string& json, const std::function<void(const std::string&)>& send,
                               Actions& actions) {
  const std::optional<std::string> type = jsonStringField(json, "type");
  if (!type) {
    send(errorJson("PROTOCOL_VERSION_MISMATCH", "Message type is missing."));
    return;
  }

  if (*type == "HELLO") {
    const std::optional<int> version = jsonIntField(json, "protocolVersion");
    if (!version || *version != kProtocolVersion) {
      send(errorJson("PROTOCOL_VERSION_MISMATCH", "ShareGuard native protocol is incompatible."));
      return;
    }
    const auto clientAt = json.find("\"client\"");
    if (clientAt != std::string::npos) {
      const auto browser = jsonStringField(json.substr(clientAt), "browser");
      if (browser) Logger::info(std::string("client browser: ") + *browser);
    }
    send(helloAckJson());
    send(processSnapshotJson(actions.snapshot(false).groups));
    return;
  }
  if (*type == "PING") {
    send("{\"type\":\"PONG\"}");
    return;
  }
  if (*type == "GET_STATUS") {
    send(actions.status());
    return;
  }
  if (*type == "GET_PROCESS_SNAPSHOT") {
    const bool includeBackground = jsonBoolField(json, "includeBackground").value_or(false);
    actions.setIncludeBackground(includeBackground);
    send(processSnapshotJson(actions.snapshot(includeBackground).groups));
    return;
  }
  if (*type == "SET_AUDIO_POLICY") {
    AudioPolicy policy;
    policy.protectionEnabled = jsonBoolField(json, "protectionEnabled").value_or(true);
    policy.blockedIds = jsonStringArray(json, "blocked");
    actions.applyPolicy(policy);
    const char* strategy = actions.strategyName();
    send(policyAppliedJson(strategy, actions.blockedCount()));
    return;
  }
  if (*type == "START_CAPTURE") {
    std::string code;
    std::string error;
    if (!actions.startCapture(code, error)) {
      send(errorJson(code.empty() ? "AUDIO_INITIALIZATION_FAILED" : code, error));
      return;
    }
    send(captureStateJson("CAPTURE_STARTED", actions.strategyName()));
    return;
  }
  if (*type == "STOP_CAPTURE") {
    actions.stopCapture();
    send(captureStateJson("CAPTURE_STOPPED", actions.strategyName()));
    return;
  }
  send(errorJson("PROTOCOL_VERSION_MISMATCH", "Unknown message type."));
}

}
