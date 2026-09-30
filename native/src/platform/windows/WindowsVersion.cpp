#include "platform/windows/WindowsVersion.hpp"

#include <windows.h>

namespace shareguard {
namespace {

using RtlGetVersionFn = LONG(WINAPI*)(OSVERSIONINFOEXW*);

}

WindowsVersion currentWindowsVersion() {
  WindowsVersion version;
  HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  if (ntdll == nullptr) {
    return version;
  }
  const auto query = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"));
  if (query == nullptr) {
    return version;
  }
  OSVERSIONINFOEXW info{};
  info.dwOSVersionInfoSize = sizeof(info);
  if (query(&info) != 0) {
    return version;
  }
  version.major = info.dwMajorVersion;
  version.minor = info.dwMinorVersion;
  version.build = info.dwBuildNumber;
  return version;
}

bool windowsMaySupportProcessLoopback() { return currentWindowsVersion().build >= 19041; }

bool windowsDocumentsProcessLoopback() { return currentWindowsVersion().build >= 20348; }

}
