#include "audio/engine/AudioMixer.hpp"

#include <algorithm>
#include <cstring>

namespace shareguard {

void AudioMixer::mix(const std::vector<AudioRingBuffer*>& inputs, float* output, size_t frames) {
  if (output == nullptr || frames == 0) {
    return;
  }
  std::fill(output, output + frames * 2, 0.0f);
  scratch_.resize(frames * 2);
  for (AudioRingBuffer* input : inputs) {
    if (input == nullptr) {
      continue;
    }
    const size_t got = input->pull(scratch_.data(), frames);
    for (size_t index = 0; index < got * 2; ++index) {
      output[index] += scratch_[index];
    }
  }
  for (size_t index = 0; index < frames * 2; ++index) {
    output[index] = std::clamp(output[index], -1.0f, 1.0f);
  }
}

}
