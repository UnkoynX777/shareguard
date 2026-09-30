#include "app/Application.hpp"

#include "audio/buffer/AudioRingBuffer.hpp"
#include "audio/engine/AudioEngine.hpp"
#include "audio/engine/AudioPolicyEngine.hpp"
#include "audio/format/WavWriter.hpp"
#include "logging/Logger.hpp"
#include "messaging/MessageDispatcher.hpp"
#include "messaging/NativeMessagingHost.hpp"
#include "messaging/Protocol.hpp"
#include "platform/windows/ComRuntime.hpp"
#include "platform/windows/WinHandle.hpp"
#include "process/ProcessCatalog.hpp"
#include "process/ProcessIdentity.hpp"
#include "process/ProcessMonitor.hpp"
#include "session/ShareSession.hpp"
#include "state/RuntimeState.hpp"

#include <windows.h>

#include <audioclient.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>

#include <wrl/client.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <io.h>
#include <stdio.h>

namespace shareguard {
namespace {

int audioSelfTest() {
  AudioRingBuffer ring(8000);
  int cursor = 0;
  const int chunks[] = {480, 480, 240, 720, 960};
  size_t chunkIndex = 0;
  auto pushChunk = [&](int frames) {
    std::vector<float> chunk(static_cast<size_t>(frames) * 2);
    for (int frame = 0; frame < frames; ++frame) {
      chunk[static_cast<size_t>(frame) * 2] = static_cast<float>(cursor);
      chunk[static_cast<size_t>(frame) * 2 + 1] = static_cast<float>(cursor);
      ++cursor;
    }
    ring.push(chunk.data(), static_cast<size_t>(frames));
  };
  std::vector<float> shortBlock(960 * 2, -1.0f);
  pushChunk(480);
  if (ring.pull(shortBlock.data(), 960) != 480) {
    Logger::error("audio self-test rejected a short pull");
    return 1;
  }
  for (int frame = 0; frame < 480; ++frame) {
    if (shortBlock[static_cast<size_t>(frame) * 2] != static_cast<float>(frame)) {
      Logger::error("audio self-test lost the first partial packet");
      return 1;
    }
  }
  std::vector<float> output;
  int packets = 0;
  while (packets < 2) {
    if (ring.available() < 960) {
      if (chunkIndex >= 5) {
        Logger::error("audio self-test ran out of input");
        return 1;
      }
      pushChunk(chunks[chunkIndex++]);
      continue;
    }
    std::vector<float> block(960 * 2);
    if (ring.pull(block.data(), 960) != 960) {
      Logger::error("audio self-test split a full packet");
      return 1;
    }
    output.insert(output.end(), block.begin(), block.end());
    ++packets;
  }
  if (output.size() != 960 * 2 * 2) {
    Logger::error("audio self-test produced the wrong packet count");
    return 1;
  }
  for (int frame = 480; frame < 480 + 1920; ++frame) {
    if (output[static_cast<size_t>(frame - 480) * 2] != static_cast<float>(frame)) {
      Logger::error("audio self-test dropped or padded samples");
      return 1;
    }
  }
  Logger::info("audio self-test passed");
  return 0;
}

void renderTone(std::stop_token stop) {
  ComRuntime com;
  if (!com.ok()) {
    return;
  }
  Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
  if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)))) {
    return;
  }
  Microsoft::WRL::ComPtr<IMMDevice> device;
  if (FAILED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device))) {
    return;
  }
  Microsoft::WRL::ComPtr<IAudioClient> client;
  if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, &client))) {
    return;
  }
  WAVEFORMATEX* mix = nullptr;
  if (FAILED(client->GetMixFormat(&mix)) || mix == nullptr) {
    Logger::error("tone renderer failed to read the mix format");
    return;
  }
  const DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
  if (FAILED(client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, 0, 0, mix, nullptr))) {
    Logger::error("tone renderer failed to open the default endpoint");
    CoTaskMemFree(mix);
    return;
  }
  const bool floatFormat = mix->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
                           (mix->wFormatTag == WAVE_FORMAT_EXTENSIBLE && mix->wBitsPerSample == 32);
  const int channels = mix->nChannels > 0 ? mix->nChannels : 2;
  const int rate = mix->nSamplesPerSec > 0 ? static_cast<int>(mix->nSamplesPerSec) : 48000;
  Logger::info("tone format channels=" + std::to_string(channels) + " rate=" + std::to_string(rate) +
               " bits=" + std::to_string(mix->wBitsPerSample) + " tag=" + std::to_string(mix->wFormatTag));
  CoTaskMemFree(mix);
  Microsoft::WRL::ComPtr<IAudioRenderClient> render;
  if (FAILED(client->GetService(IID_PPV_ARGS(&render)))) {
    return;
  }
  UINT32 bufferFrames = 0;
  client->GetBufferSize(&bufferFrames);
  ScopedHandle event(CreateEventW(nullptr, FALSE, FALSE, nullptr));
  if (!event || FAILED(client->SetEventHandle(event.get())) || FAILED(client->Start())) {
    return;
  }
  Logger::info("tone renderer 440Hz buffer=" + std::to_string(bufferFrames));
  Microsoft::WRL::ComPtr<IAudioMeterInformation> meter;
  client->GetService(IID_PPV_ARGS(&meter));
  double phase = 0.0;
  const double step = 2.0 * 3.141592653589793 * 440.0 / static_cast<double>(rate);
  std::uint64_t rendered = 0;
  bool loggedMeter = false;
  while (!stop.stop_requested()) {
    UINT32 padding = 0;
    if (FAILED(client->GetCurrentPadding(&padding)) || padding > bufferFrames) {
      break;
    }
    const UINT32 available = bufferFrames - padding;
    if (available == 0) {
      WaitForSingleObject(event.get(), 20);
      continue;
    }
    BYTE* data = nullptr;
    if (FAILED(render->GetBuffer(available, &data)) || data == nullptr) {
      break;
    }
    for (UINT32 frame = 0; frame < available; ++frame) {
      const float value = static_cast<float>(std::sin(phase) * 0.45);
      if (floatFormat) {
        auto* samples = reinterpret_cast<float*>(data);
        for (int channel = 0; channel < channels; ++channel) {
          samples[static_cast<size_t>(frame) * static_cast<size_t>(channels) + static_cast<size_t>(channel)] = value;
        }
      } else {
        auto* samples = reinterpret_cast<std::int16_t*>(data);
        const auto sample = static_cast<std::int16_t>(value * 32767.0f);
        for (int channel = 0; channel < channels; ++channel) {
          samples[static_cast<size_t>(frame) * static_cast<size_t>(channels) + static_cast<size_t>(channel)] = sample;
        }
      }
      phase += step;
      if (phase > 6.283185307179586) {
        phase -= 6.283185307179586;
      }
    }
    render->ReleaseBuffer(available, 0);
    rendered += available;
    if (!loggedMeter && rendered > 48000 && meter) {
      float peak = 0.0f;
      meter->GetPeakValue(&peak);
      Logger::info("tone meter=" + std::to_string(peak));
      loggedMeter = true;
    }
  }
  Logger::info("tone rendered=" + std::to_string(rendered));
  client->Stop();
}

int captureWav(const std::wstring& path, int seconds, const std::vector<std::string>& blocked, bool tone) {
  ComRuntime com;
  ProcessCatalog catalog;
  AudioPolicy policy;
  policy.protectionEnabled = true;
  policy.blockedIds = blocked;
  WavWriter writer;
  std::string error;
  if (!writer.open(path, error)) {
    Logger::error(error);
    return 1;
  }
  PROCESS_INFORMATION toneProcess{};
  if (tone) {
    wchar_t self[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring command = L"\"";
    command += self;
    command += L"\" --debug --tone-only --seconds ";
    command += std::to_wstring(seconds + 2);
    std::vector<wchar_t> buffer(command.begin(), command.end());
    buffer.push_back(L'\0');
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    if (!CreateProcessW(nullptr, buffer.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &toneProcess)) {
      Logger::error("tone process failed to start");
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }
  }
  AudioEngine engine;
  std::atomic<bool> failed{false};
  const bool started = engine.start(
      policy, [&] { return catalog.snapshot(true); },
      [&](const AudioFrame& frame) {
        std::string writeError;
        writer.write(frame.pcm.data(), frame.pcm.size(), writeError);
      },
      [&](const std::string& code, const std::string& message) {
        Logger::error(code + " " + message);
        failed.store(true);
      },
      error);
  if (!started) {
    Logger::error(error);
    writer.close(error);
    return 1;
  }
  Logger::info(std::string("strategy changed: ") + captureStrategyName(engine.strategy()));
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
  engine.stop();
  if (toneProcess.hProcess != nullptr) {
    WaitForSingleObject(toneProcess.hProcess, 4000);
    CloseHandle(toneProcess.hThread);
    CloseHandle(toneProcess.hProcess);
  }
  writer.close(error);
  return failed.load() ? 1 : 0;
}

int serve() {
  ComRuntime com;
  ProcessCatalog catalog;
  AudioEngine engine;
  AudioPolicyEngine policyEngine;
  ShareSession session;
  AudioPolicy policy;
  RuntimeState runtime;
  std::mutex snapshotMutex;
  ProcessSnapshot latest;
  std::vector<ApplicationGroup> published;
  bool accepted = false;
  std::atomic<bool> includeBackground{false};
  NativeMessagingHost host;

  auto currentSnapshot = [&] {
    std::lock_guard<std::mutex> lock(snapshotMutex);
    if (latest.groups.empty()) {
      latest = catalog.snapshot(includeBackground.load());
    }
    return latest;
  };

  ProcessMonitor monitor(catalog, [&](const ProcessSnapshot& snapshot) {
    std::string diff;
    {
      std::lock_guard<std::mutex> lock(snapshotMutex);
      latest = snapshot;
      if (!accepted) {
        return;
      }
      diff = processDiffJson(published, snapshot.groups);
      published = snapshot.groups;
    }
    engine.noteSnapshot();
    if (!diff.empty()) {
      host.send(diff);
    }
  });
  monitor.setIncludeBackground(false);

  MessageDispatcher dispatcher;
  MessageDispatcher::Actions actions;
  actions.snapshot = [&](bool include) {
    includeBackground.store(include);
    monitor.setIncludeBackground(include);
    std::lock_guard<std::mutex> lock(snapshotMutex);
    latest = catalog.snapshot(include);
    published = latest.groups;
    accepted = true;
    return latest;
  };
  actions.setIncludeBackground = [&](bool include) {
    includeBackground.store(include);
    monitor.setIncludeBackground(include);
  };
  actions.applyPolicy = [&](const AudioPolicy& incoming) {
    policy = incoming;
    runtime.protectionEnabled = incoming.protectionEnabled;
    runtime.blockedCount = static_cast<int>(incoming.blockedIds.size());
    if (engine.capturing()) {
      engine.applyPolicy(incoming);
    }
  };
  actions.startCapture = [&](std::string& code, std::string& error) {
    const bool started = engine.start(
        policy, currentSnapshot,
        [&](const AudioFrame& frame) { host.send(audioFrameJson(frame)); },
        [&](const std::string& failureCode, const std::string& message) {
          runtime.lastError = message;
          host.send(errorJson(failureCode, message));
        },
        error);
    if (!started) {
      code = engine.failureCode().empty() ? "AUDIO_INITIALIZATION_FAILED" : engine.failureCode();
      runtime.lastError = error;
      session.setActive(false);
      return false;
    }
    session.setActive(true);
    runtime.lastError.clear();
    runtime.captureStrategy = engine.strategy();
    return true;
  };
  actions.stopCapture = [&] {
    session.setActive(false);
    engine.stop();
  };
  actions.strategyName = [&] {
    return captureStrategyName(policyEngine.plan(currentSnapshot(), policy).strategy);
  };
  actions.blockedCount = [&] { return static_cast<int>(policy.blockedIds.size()); };
  actions.status = [&] {
    runtime.capturing = engine.capturing();
    runtime.sharing = session.active();
    runtime.captureStrategy = policyEngine.plan(currentSnapshot(), policy).strategy;
    runtime.activeSourceCount = engine.sourceCount();
    runtime.blockedCount = static_cast<int>(policy.blockedIds.size());
    return statusJson(runtime.capturing, runtime.sharing, runtime.protectionEnabled,
                      captureStrategyName(runtime.captureStrategy), runtime.blockedCount, runtime.activeSourceCount,
                      runtime.lastError);
  };

  host.run([&](const std::string& json) { dispatcher.handle(json, [&](const std::string& message) { host.send(message); }, actions); });
  engine.stop();
  return 0;
}

}

int Application::run(int argc, wchar_t** argv) {
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

  bool debug = GetEnvironmentVariableW(L"SHAREGUARD_DEBUG", nullptr, 0) > 0;
  bool capture = false;
  bool selfTest = false;
  bool tone = false;
  bool toneOnly = false;
  std::wstring wavPath;
  int seconds = 5;
  std::vector<std::string> blocked;
  for (int index = 1; index < argc; ++index) {
    const std::wstring arg = argv[index];
    if (arg == L"--debug") {
      debug = true;
    } else if (arg == L"--audio-self-test") {
      selfTest = true;
    } else if (arg == L"--tone") {
      tone = true;
    } else if (arg == L"--tone-only") {
      toneOnly = true;
    } else if (arg == L"--capture-wav" && index + 1 < argc) {
      capture = true;
      wavPath = argv[++index];
    } else if (arg == L"--seconds" && index + 1 < argc) {
      seconds = _wtoi(argv[++index]);
    } else if (arg == L"--block" && index + 1 < argc) {
      blocked.push_back(processIdentity(argv[++index]));
    }
  }
  if (seconds < 1) {
    seconds = 1;
  }
  if (seconds > 30) {
    seconds = 30;
  }
  Logger::setDebug(debug || capture || selfTest || toneOnly);
  if (selfTest) {
    return audioSelfTest();
  }
  if (toneOnly) {
    std::jthread toneThread([](std::stop_token stop) { renderTone(stop); });
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    return 0;
  }
  if (capture) {
    return captureWav(wavPath, seconds, blocked, tone);
  }
  return serve();
}

}
