#include "audio/format/AudioFrame.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace shareguard {

void quantizeStereo(const float* interleaved, int frames, std::vector<std::uint8_t>& pcm) {
  pcm.resize(static_cast<size_t>(frames) * kCanonicalChannels * sizeof(std::int16_t));
  auto* output = reinterpret_cast<std::int16_t*>(pcm.data());
  for (int index = 0; index < frames * kCanonicalChannels; ++index) {
    const float scaled = std::clamp(interleaved[index], -1.0f, 1.0f) * 32767.0f;
    output[index] = static_cast<std::int16_t>(std::lrintf(scaled));
  }
}

}
