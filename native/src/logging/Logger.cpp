#include "logging/Logger.hpp"

#include <windows.h>

#include <atomic>
#include <chrono>
#include <ctime>
#include <fstream>
#include <mutex>

namespace shareguard {
namespace {

std::atomic<bool> debugEnabled{false};
std::mutex writeMutex;

std::string timestamp() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t time = std::chrono::system_clock::to_time_t(now);
  std::tm local{};
  localtime_s(&local, &time);
  char buffer[32] = {};
  std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &local);
  return buffer;
}

void appendFile(const std::string& line) {
  wchar_t localAppData[MAX_PATH] = {};
  const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
  if (length == 0 || length >= MAX_PATH) {
    return;
  }
  const std::wstring directory = std::wstring(localAppData) + L"\\ShareGuard";
  CreateDirectoryW(directory.c_str(), nullptr);
  const std::wstring path = directory + L"\\shareguard.log";
  WIN32_FILE_ATTRIBUTE_DATA data{};
  if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data)) {
    const ULONGLONG size = (static_cast<ULONGLONG>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
    if (size > 1024 * 1024) {
      DeleteFileW(path.c_str());
    }
  }
  std::ofstream output(path, std::ios::app | std::ios::binary);
  if (output) {
    output << line << "\n";
  }
}

}

void Logger::setDebug(bool enabled) { debugEnabled.store(enabled); }

void Logger::info(const std::string& message) { write(message, false); }

void Logger::error(const std::string& message) { write(message, true); }

void Logger::write(const std::string& message, bool always) {
  if (!always && !debugEnabled.load()) {
    return;
  }
  const std::string line = timestamp() + " " + message + "\r\n";
  std::lock_guard<std::mutex> lock(writeMutex);
  const HANDLE errorOutput = GetStdHandle(STD_ERROR_HANDLE);
  if (errorOutput != nullptr && errorOutput != INVALID_HANDLE_VALUE) {
    DWORD written = 0;
    WriteFile(errorOutput, line.data(), static_cast<DWORD>(line.size()), &written, nullptr);
  }
  appendFile(line);
}

}
