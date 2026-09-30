#pragma once

#include "audio/format/AudioFrame.hpp"
#include "policy/AudioPolicy.hpp"
#include "process/ProcessModel.hpp"

#include <optional>
#include <string>
#include <vector>

namespace shareguard {

constexpr int kProtocolVersion = 2;
constexpr const char* kNativeVersion = "0.3.0";

std::optional<std::string> jsonStringField(const std::string& json, const std::string& key);
std::optional<bool> jsonBoolField(const std::string& json, const std::string& key);
std::optional<int> jsonIntField(const std::string& json, const std::string& key);
std::vector<std::string> jsonStringArray(const std::string& json, const std::string& key);
std::string normalizeIdentity(std::string identity);

std::string helloAckJson();
std::string processSnapshotJson(const std::vector<ApplicationGroup>& groups);
std::string processDiffJson(const std::vector<ApplicationGroup>& previous, const std::vector<ApplicationGroup>& next);
std::string audioFrameJson(const AudioFrame& frame);
std::string errorJson(const std::string& code, const std::string& message);
std::string policyAppliedJson(const char* strategy, int blockedCount);
std::string captureStateJson(const char* type, const char* strategy);
std::string statusJson(bool capturing, bool sharing, bool protectionEnabled, const char* strategy, int blockedCount,
                       int activeSourceCount, const std::string& lastError);

}
