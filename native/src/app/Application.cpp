#include "app/Application.hpp"

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

#include <atomic>
#include <chrono>
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

int captureWav(const std::wstring& path, int seconds, const std::vector<std::string>& blocked) {
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
  std::wstring wavPath;
  int seconds = 5;
  std::vector<std::string> blocked;
  for (int index = 1; index < argc; ++index) {
    const std::wstring arg = argv[index];
    if (arg == L"--debug") {
      debug = true;
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
  Logger::setDebug(debug || capture);
  if (capture) {
    return captureWav(wavPath, seconds, blocked);
  }
  return serve();
}

}
