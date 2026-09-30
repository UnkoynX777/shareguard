#pragma once

#include "platform/windows/WinHandle.hpp"

#include <cstdint>
#include <string>

namespace shareguard {

class WavWriter {
 public:
  WavWriter() = default;
  ~WavWriter();

  WavWriter(const WavWriter&) = delete;
  WavWriter& operator=(const WavWriter&) = delete;

  bool open(const std::wstring& path, std::string& error);
  bool write(const uint8_t* pcm, size_t size, std::string& error);
  bool close(std::string& error);

 private:
  ScopedHandle file_;
  uint32_t dataBytes_ = 0;
  bool open_ = false;
};

}
