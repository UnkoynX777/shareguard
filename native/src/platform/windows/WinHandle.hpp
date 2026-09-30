#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <string>

namespace shareguard {

class ScopedHandle {
 public:
  ScopedHandle() = default;
  explicit ScopedHandle(HANDLE handle) : handle_(handle) {}
  ~ScopedHandle() { reset(); }

  ScopedHandle(const ScopedHandle&) = delete;
  ScopedHandle& operator=(const ScopedHandle&) = delete;

  ScopedHandle(ScopedHandle&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }

  ScopedHandle& operator=(ScopedHandle&& other) noexcept {
    if (this != &other) {
      reset(other.handle_);
      other.handle_ = nullptr;
    }
    return *this;
  }

  void reset(HANDLE handle = nullptr) {
    if (handle_ && handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(handle_);
    }
    handle_ = handle;
  }

  HANDLE get() const { return handle_; }
  explicit operator bool() const { return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE; }

 private:
  HANDLE handle_ = nullptr;
};

std::string hresultText(const char* action, HRESULT result);
std::string jsonEscape(const std::string& value);
std::string wideToUtf8(const std::wstring& value);
std::wstring utf8ToWide(const std::string& value);
std::wstring toLowerWide(std::wstring value);

}
