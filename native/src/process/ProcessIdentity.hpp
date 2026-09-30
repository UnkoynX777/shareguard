#pragma once

#include <string>

namespace shareguard {

std::string processIdentity(const std::wstring& executableName);
std::string displayNameForPath(const std::wstring& executablePath, const std::wstring& executableName);

}
