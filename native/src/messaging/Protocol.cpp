#include "messaging/Protocol.hpp"

#include "platform/windows/WinHandle.hpp"
#include "platform/windows/WindowsVersion.hpp"

#include <cctype>

namespace shareguard {
namespace {

std::string base64Encode(const uint8_t* data, size_t size) {
  static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve(((size + 2) / 3) * 4);
  size_t index = 0;
  while (index + 3 <= size) {
    const unsigned value = (static_cast<unsigned>(data[index]) << 16) |
                           (static_cast<unsigned>(data[index + 1]) << 8) | data[index + 2];
    out.push_back(kAlphabet[(value >> 18) & 63]);
    out.push_back(kAlphabet[(value >> 12) & 63]);
    out.push_back(kAlphabet[(value >> 6) & 63]);
    out.push_back(kAlphabet[value & 63]);
    index += 3;
  }
  if (index < size) {
    unsigned value = static_cast<unsigned>(data[index]) << 16;
    out.push_back(kAlphabet[(value >> 18) & 63]);
    if (index + 1 < size) {
      value |= static_cast<unsigned>(data[index + 1]) << 8;
      out.push_back(kAlphabet[(value >> 12) & 63]);
      out.push_back(kAlphabet[(value >> 6) & 63]);
      out.push_back('=');
    } else {
      out.push_back(kAlphabet[(value >> 12) & 63]);
      out.push_back('=');
      out.push_back('=');
    }
  }
  return out;
}

std::string groupJson(const ApplicationGroup& group) {
  std::string json = "{\"id\":\"" + jsonEscape(group.id) + "\"";
  json += ",\"rootPid\":" + std::to_string(group.rootPids.empty() ? 0 : group.rootPids.front());
  json += ",\"name\":\"" + jsonEscape(group.displayName) + "\"";
  json += ",\"executable\":\"" + jsonEscape(group.executableName) + "\"";
  json += ",\"audioActive\":";
  json += group.audioActive ? "true" : "false";
  json += ",\"processCount\":" + std::to_string(group.pids.size()) + "}";
  return json;
}

const ApplicationGroup* findGroup(const std::vector<ApplicationGroup>& groups, const std::string& id) {
  for (const ApplicationGroup& group : groups) {
    if (group.id == id) {
      return &group;
    }
  }
  return nullptr;
}

bool sameGroup(const ApplicationGroup& left, const ApplicationGroup& right) {
  const std::uint32_t leftRoot = left.rootPids.empty() ? 0 : left.rootPids.front();
  const std::uint32_t rightRoot = right.rootPids.empty() ? 0 : right.rootPids.front();
  return left.displayName == right.displayName && left.audioActive == right.audioActive && leftRoot == rightRoot &&
         left.pids.size() == right.pids.size();
}

}

std::optional<std::string> jsonStringField(const std::string& json, const std::string& key) {
  const std::string pattern = "\"" + key + "\"";
  const size_t keyPos = json.find(pattern);
  if (keyPos == std::string::npos) {
    return std::nullopt;
  }
  const size_t colon = json.find(':', keyPos + pattern.size());
  if (colon == std::string::npos) {
    return std::nullopt;
  }
  const size_t open = json.find('"', colon + 1);
  if (open == std::string::npos) {
    return std::nullopt;
  }
  std::string value;
  for (size_t index = open + 1; index < json.size(); ++index) {
    const char character = json[index];
    if (character == '\\' && index + 1 < json.size()) {
      value.push_back(json[++index]);
      continue;
    }
    if (character == '"') {
      return value;
    }
    value.push_back(character);
  }
  return std::nullopt;
}

std::optional<bool> jsonBoolField(const std::string& json, const std::string& key) {
  const std::string pattern = "\"" + key + "\"";
  const size_t keyPos = json.find(pattern);
  if (keyPos == std::string::npos) {
    return std::nullopt;
  }
  const size_t colon = json.find(':', keyPos + pattern.size());
  if (colon == std::string::npos) {
    return std::nullopt;
  }
  const size_t truePos = json.find("true", colon + 1);
  const size_t falsePos = json.find("false", colon + 1);
  if (truePos != std::string::npos && (falsePos == std::string::npos || truePos < falsePos)) {
    return true;
  }
  if (falsePos != std::string::npos) {
    return false;
  }
  return std::nullopt;
}

std::optional<int> jsonIntField(const std::string& json, const std::string& key) {
  const std::string pattern = "\"" + key + "\"";
  const size_t keyPos = json.find(pattern);
  if (keyPos == std::string::npos) {
    return std::nullopt;
  }
  const size_t colon = json.find(':', keyPos + pattern.size());
  if (colon == std::string::npos) {
    return std::nullopt;
  }
  size_t index = colon + 1;
  while (index < json.size() && (json[index] == ' ' || json[index] == '\t')) {
    ++index;
  }
  try {
    size_t consumed = 0;
    const int value = std::stoi(json.substr(index), &consumed);
    return value;
  } catch (...) {
    return std::nullopt;
  }
}

std::vector<std::string> jsonStringArray(const std::string& json, const std::string& key) {
  std::vector<std::string> values;
  const std::string pattern = "\"" + key + "\"";
  const size_t keyPos = json.find(pattern);
  if (keyPos == std::string::npos) {
    return values;
  }
  const size_t open = json.find('[', keyPos + pattern.size());
  if (open == std::string::npos) {
    return values;
  }
  for (size_t index = open + 1; index < json.size(); ++index) {
    if (json[index] == ']') {
      break;
    }
    if (json[index] != '"') {
      continue;
    }
    std::string value;
    for (++index; index < json.size(); ++index) {
      if (json[index] == '\\' && index + 1 < json.size()) {
        value.push_back(json[++index]);
        continue;
      }
      if (json[index] == '"') {
        break;
      }
      value.push_back(json[index]);
    }
    values.push_back(normalizeIdentity(value));
  }
  return values;
}

std::string normalizeIdentity(std::string identity) {
  for (char& character : identity) {
    character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
  }
  return identity;
}

std::string helloAckJson() {
  return std::string("{\"type\":\"HELLO_ACK\",\"protocolVersion\":") + std::to_string(kProtocolVersion) +
         ",\"nativeVersion\":\"" + kNativeVersion + "\",\"processLoopbackSupported\":" +
         (windowsMaySupportProcessLoopback() ? "true" : "false") + "}";
}

std::string processSnapshotJson(const std::vector<ApplicationGroup>& groups) {
  std::string json = "{\"type\":\"PROCESS_SNAPSHOT\",\"processes\":[";
  for (size_t index = 0; index < groups.size(); ++index) {
    if (index > 0) {
      json += ',';
    }
    json += groupJson(groups[index]);
  }
  json += "]}";
  return json;
}

std::string processDiffJson(const std::vector<ApplicationGroup>& previous, const std::vector<ApplicationGroup>& next) {
  std::string added;
  std::string updated;
  std::string removed;
  for (const ApplicationGroup& group : next) {
    const ApplicationGroup* old = findGroup(previous, group.id);
    if (old == nullptr) {
      if (!added.empty()) {
        added += ',';
      }
      added += groupJson(group);
    } else if (!sameGroup(*old, group)) {
      if (!updated.empty()) {
        updated += ',';
      }
      updated += groupJson(group);
    }
  }
  for (const ApplicationGroup& group : previous) {
    if (findGroup(next, group.id) == nullptr) {
      if (!removed.empty()) {
        removed += ',';
      }
      removed += "\"" + jsonEscape(group.id) + "\"";
    }
  }
  if (added.empty() && updated.empty() && removed.empty()) {
    return {};
  }
  return "{\"type\":\"PROCESS_DIFF\",\"added\":[" + added + "],\"removed\":[" + removed + "],\"updated\":[" + updated +
         "]}";
}

std::string audioFrameJson(const AudioFrame& frame) {
  const std::string payload = base64Encode(frame.pcm.data(), frame.pcm.size());
  std::string json;
  json.reserve(payload.size() + 128);
  json += "{\"type\":\"AUDIO_FRAME\",\"sequence\":";
  json += std::to_string(frame.sequence);
  json += ",\"sampleRate\":";
  json += std::to_string(frame.sampleRate);
  json += ",\"channels\":";
  json += std::to_string(frame.channels);
  json += ",\"format\":\"s16le\",\"data\":\"";
  json += payload;
  json += "\"}";
  return json;
}

std::string errorJson(const std::string& code, const std::string& message) {
  return "{\"type\":\"ERROR\",\"code\":\"" + jsonEscape(code) + "\",\"message\":\"" + jsonEscape(message) + "\"}";
}

std::string policyAppliedJson(const char* strategy, int blockedCount) {
  return std::string("{\"type\":\"AUDIO_POLICY_APPLIED\",\"captureStrategy\":\"") + strategy +
         "\",\"blockedCount\":" + std::to_string(blockedCount) + "}";
}

std::string captureStateJson(const char* type, const char* strategy) {
  return std::string("{\"type\":\"") + type + "\",\"captureStrategy\":\"" + strategy + "\"}";
}

std::string statusJson(bool capturing, bool sharing, bool protectionEnabled, const char* strategy, int blockedCount,
                       int activeSourceCount, const std::string& lastError) {
  std::string json = "{\"type\":\"STATUS\",\"capturing\":";
  json += capturing ? "true" : "false";
  json += ",\"sharing\":";
  json += sharing ? "true" : "false";
  json += ",\"protectionEnabled\":";
  json += protectionEnabled ? "true" : "false";
  json += ",\"captureStrategy\":\"" + std::string(strategy) + "\"";
  json += ",\"blockedCount\":" + std::to_string(blockedCount);
  json += ",\"activeSourceCount\":" + std::to_string(activeSourceCount);
  json += ",\"processLoopbackSupported\":";
  json += windowsMaySupportProcessLoopback() ? "true" : "false";
  json += ",\"lastError\":\"" + jsonEscape(lastError) + "\"}";
  return json;
}

}
