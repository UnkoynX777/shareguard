#include "platform/windows/WinHandle.hpp"

#include <cstdio>
#include <cwctype>

namespace shareguard {

std::string hresultText(const char* action, HRESULT result) {
  char buffer[160] = {};
  std::snprintf(buffer, sizeof(buffer), "%s failed (0x%08lX)", action, static_cast<unsigned long>(result));
  return buffer;
}

std::string jsonEscape(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (const unsigned char character : value) {
    switch (character) {
      case '"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        if (character < 0x20) {
          char hex[8] = {};
          std::snprintf(hex, sizeof(hex), "\\u%04x", character);
          escaped += hex;
        } else {
          escaped.push_back(static_cast<char>(character));
        }
        break;
    }
  }
  return escaped;
}

std::string wideToUtf8(const std::wstring& value) {
  if (value.empty()) {
    return {};
  }
  const int length = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0,
                                         nullptr, nullptr);
  std::string output(static_cast<size_t>(length), '\0');
  WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), output.data(), length, nullptr,
                      nullptr);
  return output;
}

std::wstring utf8ToWide(const std::string& value) {
  if (value.empty()) {
    return {};
  }
  const int length = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
  std::wstring output(static_cast<size_t>(length), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), output.data(), length);
  return output;
}

std::wstring toLowerWide(std::wstring value) {
  for (wchar_t& character : value) {
    character = static_cast<wchar_t>(std::towlower(character));
  }
  return value;
}

}
