#include "process/ProcessIdentity.hpp"

#include "platform/windows/WinHandle.hpp"

#include <windows.h>

#include <mutex>
#include <unordered_map>
#include <vector>

namespace shareguard {
namespace {

std::mutex nameCacheMutex;
std::unordered_map<std::wstring, std::string> nameCache;

std::wstring queryVersionString(const std::wstring& path, const wchar_t* key) {
  DWORD ignored = 0;
  const DWORD size = GetFileVersionInfoSizeW(path.c_str(), &ignored);
  if (size == 0) {
    return {};
  }
  std::vector<unsigned char> data(size);
  if (!GetFileVersionInfoW(path.c_str(), 0, size, data.data())) {
    return {};
  }
  struct Translation {
    WORD language;
    WORD codePage;
  };
  Translation* translations = nullptr;
  UINT translationBytes = 0;
  if (!VerQueryValueW(data.data(), L"\\VarFileInfo\\Translation", reinterpret_cast<void**>(&translations),
                      &translationBytes) ||
      translationBytes < sizeof(Translation)) {
    return {};
  }
  wchar_t query[80] = {};
  swprintf_s(query, L"\\StringFileInfo\\%04x%04x\\%s", translations[0].language, translations[0].codePage, key);
  wchar_t* value = nullptr;
  UINT valueBytes = 0;
  if (!VerQueryValueW(data.data(), query, reinterpret_cast<void**>(&value), &valueBytes) || value == nullptr ||
      value[0] == L'\0') {
    return {};
  }
  return value;
}

}

std::string processIdentity(const std::wstring& executableName) {
  return wideToUtf8(toLowerWide(executableName));
}

std::string displayNameForPath(const std::wstring& executablePath, const std::wstring& executableName) {
  if (!executablePath.empty()) {
    std::lock_guard<std::mutex> lock(nameCacheMutex);
    const auto cached = nameCache.find(executablePath);
    if (cached != nameCache.end()) {
      return cached->second;
    }
  }

  std::wstring friendly = queryVersionString(executablePath, L"FileDescription");
  if (friendly.empty()) {
    friendly = queryVersionString(executablePath, L"ProductName");
  }
  std::string result = friendly.empty() ? wideToUtf8(executableName) : wideToUtf8(friendly);
  if (!result.empty() && (result.back() == ' ' || result.back() == '\0')) {
    while (!result.empty() && (result.back() == ' ' || result.back() == '\0')) {
      result.pop_back();
    }
  }
  if (result.empty()) {
    result = wideToUtf8(executableName);
  }
  if (!executablePath.empty()) {
    std::lock_guard<std::mutex> lock(nameCacheMutex);
    nameCache[executablePath] = result;
  }
  return result;
}

}
