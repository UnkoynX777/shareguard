#include "audio/capture/WasapiLoopbackSession.hpp"

#include "logging/Logger.hpp"
#include "platform/windows/WinHandle.hpp"

#include <objbase.h>

namespace shareguard {

bool WasapiLoopbackSession::initializeWithFormat(const WAVEFORMATEX* format, DWORD streamFlags, std::string& error) {
  const HRESULT result = client_->Initialize(AUDCLNT_SHAREMODE_SHARED, streamFlags, 0, 0, format, nullptr);
  if (FAILED(result)) {
    error = hresultText("IAudioClient::Initialize", result);
    return false;
  }
  format_ = format == &requested_ ? &requested_ : mixFormat_;
  return true;
}

bool WasapiLoopbackSession::initialize(Microsoft::WRL::ComPtr<IAudioClient> client, std::string& error) {
  stop();
  client_ = std::move(client);
  if (!client_) {
    error = "Audio client is missing.";
    return false;
  }

  requested_ = {};
  requested_.wFormatTag = WAVE_FORMAT_PCM;
  requested_.nChannels = 2;
  requested_.nSamplesPerSec = 48000;
  requested_.wBitsPerSample = 16;
  requested_.nBlockAlign = 4;
  requested_.nAvgBytesPerSec = 48000 * 4;

  const DWORD convertedFlags = AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
                               AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
  if (!initializeWithFormat(&requested_, convertedFlags, error)) {
    Logger::info(error + "; retrying with the mix format.");
    releaseInterfaces();
    return false;
  }
  return startClient(error);
}

bool WasapiLoopbackSession::initializeMix(Microsoft::WRL::ComPtr<IAudioClient> client, std::string& error) {
  stop();
  client_ = std::move(client);
  if (!client_) {
    error = "Audio client is missing.";
    return false;
  }

  const HRESULT mixResult = client_->GetMixFormat(&mixFormat_);
  if (FAILED(mixResult) || mixFormat_ == nullptr) {
    error = hresultText("IAudioClient::GetMixFormat", mixResult);
    releaseInterfaces();
    return false;
  }

  const DWORD flags = AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
  if (!initializeWithFormat(mixFormat_, flags, error)) {
    releaseInterfaces();
    return false;
  }
  return startClient(error);
}

bool WasapiLoopbackSession::startClient(std::string& error) {
  HRESULT result = client_->GetService(IID_PPV_ARGS(&capture_));
  if (FAILED(result)) {
    error = hresultText("IAudioClient::GetService", result);
    releaseInterfaces();
    return false;
  }

  sampleEvent_.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
  if (!sampleEvent_) {
    error = "Failed to create the audio sample event.";
    releaseInterfaces();
    return false;
  }

  result = client_->SetEventHandle(sampleEvent_.get());
  if (FAILED(result)) {
    error = hresultText("IAudioClient::SetEventHandle", result);
    releaseInterfaces();
    return false;
  }

  result = client_->Start();
  if (FAILED(result)) {
    error = hresultText("IAudioClient::Start", result);
    releaseInterfaces();
    return false;
  }
  started_ = true;
  return true;
}

bool WasapiLoopbackSession::read(const std::function<void(const CapturedFrames&)>& consumer, std::string& error) {
  if (!capture_) {
    error = "Capture client is not running.";
    return false;
  }

  while (true) {
    UINT32 packetFrames = 0;
    HRESULT result = capture_->GetNextPacketSize(&packetFrames);
    if (FAILED(result)) {
      error = hresultText("GetNextPacketSize", result);
      return false;
    }
    if (packetFrames == 0) {
      return true;
    }

    BYTE* data = nullptr;
    UINT32 frames = 0;
    DWORD flags = 0;
    result = capture_->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
    if (FAILED(result)) {
      error = hresultText("GetBuffer", result);
      return false;
    }

    CapturedFrames captured;
    captured.frames = frames;
    captured.silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0 || data == nullptr;
    captured.discontinuity = (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) != 0;
    captured.data = captured.silent ? nullptr : data;
    consumer(captured);

    result = capture_->ReleaseBuffer(frames);
    if (FAILED(result)) {
      error = hresultText("ReleaseBuffer", result);
      return false;
    }
  }
}

void WasapiLoopbackSession::releaseInterfaces() {
  if (started_ && client_) {
    client_->Stop();
  }
  started_ = false;
  capture_.Reset();
  client_.Reset();
  sampleEvent_.reset();
  if (mixFormat_) {
    CoTaskMemFree(mixFormat_);
    mixFormat_ = nullptr;
  }
  format_ = nullptr;
}

void WasapiLoopbackSession::stop() { releaseInterfaces(); }

}
