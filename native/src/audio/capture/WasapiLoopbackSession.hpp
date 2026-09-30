#pragma once

#include "platform/windows/WinHandle.hpp"

#include <audioclient.h>
#include <mmreg.h>
#include <wrl/client.h>

#include <functional>
#include <string>

namespace shareguard {

struct CapturedFrames {
  const BYTE* data = nullptr;
  UINT32 frames = 0;
  bool silent = false;
  bool discontinuity = false;
  UINT64 devicePosition = 0;
  UINT64 qpcPosition = 0;
};

class WasapiLoopbackSession {
 public:
  bool initialize(Microsoft::WRL::ComPtr<IAudioClient> client, std::string& error);
  bool initializeMix(Microsoft::WRL::ComPtr<IAudioClient> client, std::string& error);
  bool read(const std::function<void(const CapturedFrames&)>& consumer, std::string& error);
  void stop();

  HANDLE sampleEvent() const { return sampleEvent_.get(); }
  const WAVEFORMATEX* format() const { return format_; }

 private:
  bool initializeWithFormat(const WAVEFORMATEX* format, DWORD streamFlags, std::string& error);
  bool startClient(std::string& error);
  void releaseInterfaces();

  Microsoft::WRL::ComPtr<IAudioClient> client_;
  Microsoft::WRL::ComPtr<IAudioCaptureClient> capture_;
  ScopedHandle sampleEvent_;
  WAVEFORMATEX requested_{};
  WAVEFORMATEX* format_ = nullptr;
  WAVEFORMATEX* mixFormat_ = nullptr;
  bool started_ = false;
};

}
