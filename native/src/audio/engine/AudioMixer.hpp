#pragma once

#include "audio/buffer/AudioRingBuffer.hpp"

#include <cstddef>
#include <vector>

namespace shareguard {

class AudioMixer {
 public:
  void mix(const std::vector<AudioRingBuffer*>& inputs, float* output, size_t frames);

 private:
  std::vector<float> scratch_;
};

}
