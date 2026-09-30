#pragma once

#include <cstdint>

namespace shareguard {

struct WindowsVersion {
  std::uint32_t major = 0;
  std::uint32_t minor = 0;
  std::uint32_t build = 0;
};

WindowsVersion currentWindowsVersion();
bool windowsMaySupportProcessLoopback();
bool windowsDocumentsProcessLoopback();

}
