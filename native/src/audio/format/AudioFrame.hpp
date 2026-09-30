#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace shareguard {

constexpr int kCanonicalRate = 48000;
constexpr int kCanonicalChannels = 2;
constexpr int kPacketFrames = 960;

struct AudioFrame {
  std::uint64_t sequence = 0;
  int sampleRate = kCanonicalRate;
  int channels = kCanonicalChannels;
  int frameCount = kPacketFrames;
  std::vector<std::uint8_t> pcm;
};

void quantizeStereo(const float* interleaved, int frames, std::vector<std::uint8_t>& pcm);

}
