#include "messaging/NativeMessagingHost.hpp"

#include "logging/Logger.hpp"

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <mutex>

namespace shareguard {
namespace {

bool readExact(void* data, DWORD size) {
  auto* bytes = static_cast<uint8_t*>(data);
  DWORD remaining = size;
  while (remaining > 0) {
    DWORD read = 0;
    if (!ReadFile(GetStdHandle(STD_INPUT_HANDLE), bytes, remaining, &read, nullptr) || read == 0) {
      return false;
    }
    bytes += read;
    remaining -= read;
  }
  return true;
}

}

void NativeMessagingHost::send(const std::string& json) {
  if (broken_.load() || json.size() > 1024 * 1024) {
    return;
  }
  const uint32_t length = static_cast<uint32_t>(json.size());
  std::lock_guard<std::mutex> lock(writeMutex_);
  const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD written = 0;
  if (!WriteFile(output, &length, sizeof(length), &written, nullptr) || written != sizeof(length)) {
    broken_.store(true);
    return;
  }
  DWORD remaining = length;
  const char* bytes = json.data();
  while (remaining > 0) {
    if (!WriteFile(output, bytes, remaining, &written, nullptr) || written == 0) {
      broken_.store(true);
      return;
    }
    bytes += written;
    remaining -= written;
  }
}

void NativeMessagingHost::run(const Handler& handler) {
  while (!broken_.load()) {
    uint32_t length = 0;
    if (!readExact(&length, sizeof(length))) {
      break;
    }
    if (length == 0 || length > 1024 * 1024) {
      Logger::error("Rejected a native message with an invalid length.");
      break;
    }
    std::string json(length, '\0');
    if (!readExact(json.data(), length)) {
      break;
    }
    if (handler) {
      handler(json);
    }
  }
  broken_.store(true);
}

}
