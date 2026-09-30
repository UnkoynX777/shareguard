#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmreg.h>

#include <cstdint>
#include <string>
#include <vector>

namespace shareguard {

class AudioConverter {
 public:
  bool prepare(const WAVEFORMATEX* format, std::string& error);
  void convert(const BYTE* data, UINT32 frames, bool silent, std::vector<float>& interleavedStereo);
  void reset();

 private:
  float readSample(const uint8_t* frame, int channel) const;
  void appendSourceFrame(float left, float right, std::vector<float>& output);

  int sourceRate_ = 0;
  int sourceChannels_ = 0;
  int containerBits_ = 0;
  int blockAlign_ = 0;
  bool isFloat_ = false;
  std::vector<float> fifo_;
  double position_ = 0.0;
};

}
