#pragma once

#include "audio/capture/WasapiLoopbackSession.hpp"

#include <cstdint>
#include <string>

namespace shareguard {

enum class ProcessLoopbackMode { IncludeTree, ExcludeTree };

class ProcessLoopbackCapture {
 public:
  bool open(std::uint32_t processId, ProcessLoopbackMode mode, WasapiLoopbackSession& session, std::string& error);
};

class SystemLoopbackCapture {
 public:
  bool open(WasapiLoopbackSession& session, std::string& error);
};

}
