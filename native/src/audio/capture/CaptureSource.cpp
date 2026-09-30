#include "audio/capture/CaptureSource.hpp"

#include "logging/Logger.hpp"
#include "platform/windows/ComRuntime.hpp"

#include <avrt.h>
#include <chrono>

namespace shareguard {

CaptureSource::~CaptureSource() { stop(); }

bool CaptureSource::startSystem(std::string& error) { return start(false, 0, false, error); }

bool CaptureSource::startProcess(std::uint32_t processId, bool excludeTree, std::string& error) {
  return start(true, processId, excludeTree, error);
}

bool CaptureSource::start(bool process, std::uint32_t processId, bool excludeTree, std::string& error) {
  stop();
  auto open = std::make_shared<OpenResult>();
  thread_ = std::jthread([this, process, processId, excludeTree, open](std::stop_token stop) {
    run(stop, process, processId, excludeTree, open);
  });
  std::unique_lock<std::mutex> lock(open->mutex);
  open->ready.wait_for(lock, std::chrono::seconds(8), [&] { return open->finished; });
  if (open->ok) {
    return true;
  }
  error = open->error.empty() ? "Capture source failed to start." : open->error;
  lock.unlock();
  stop();
  return false;
}

void CaptureSource::stop() {
  if (thread_.joinable()) {
    thread_.request_stop();
    thread_.join();
  }
  running_.store(false);
}

void CaptureSource::run(std::stop_token stop, bool process, std::uint32_t processId, bool excludeTree,
                        const std::shared_ptr<OpenResult>& open) {
  ComRuntime com;
  DWORD taskIndex = 0;
  HANDLE mmcss = AvSetMmThreadCharacteristicsW(L"Audio", &taskIndex);
  auto finish = [&](bool ok, const std::string& message) {
    std::lock_guard<std::mutex> lock(open->mutex);
    if (!open->finished) {
      open->ok = ok;
      open->error = message;
      open->finished = true;
      open->ready.notify_all();
    }
  };

  if (!com.ok()) {
    finish(false, "COM initialization failed.");
    if (mmcss != nullptr) {
      AvRevertMmThreadCharacteristics(mmcss);
    }
    return;
  }

  std::string error;
  bool opened = false;
  if (process) {
    ProcessLoopbackCapture capture;
    opened = capture.open(processId, excludeTree ? ProcessLoopbackMode::ExcludeTree : ProcessLoopbackMode::IncludeTree,
                          session_, error);
  } else {
    SystemLoopbackCapture capture;
    opened = capture.open(session_, error);
  }
  if (!opened || !converter_.prepare(session_.format(), error)) {
    session_.stop();
    finish(false, error.empty() ? "Audio capture failed." : error);
    if (mmcss != nullptr) {
      AvRevertMmThreadCharacteristics(mmcss);
    }
    return;
  }

  running_.store(true);
  sampleRate_.store(static_cast<int>(session_.format()->nSamplesPerSec));
  finish(true, {});
  while (!stop.stop_requested()) {
    WaitForSingleObject(session_.sampleEvent(), 200);
    if (stop.stop_requested()) {
      break;
    }
    std::string readError;
    const bool ok = session_.read(
        [&](const CapturedFrames& frames) {
          if (frames.discontinuity) {
            discontinuities_.fetch_add(1);
          }
          if (frames.devicePosition != 0) {
            if (lastDeviceValid_ && frames.devicePosition > lastDeviceEnd_ &&
                frames.devicePosition - lastDeviceEnd_ > frames.frames) {
              discontinuities_.fetch_add(1);
            }
            lastDeviceEnd_ = frames.devicePosition + frames.frames;
            lastDeviceValid_ = true;
          }
          convertScratch_.clear();
          converter_.convert(frames.data, frames.frames, frames.silent, convertScratch_);
          if (convertScratch_.size() >= 2) {
            const size_t count = convertScratch_.size() / 2;
            capturedFrames_.fetch_add(count);
            buffer_.push(convertScratch_.data(), count);
          }
        },
        readError);
    if (!ok) {
      Logger::error(readError);
      break;
    }
  }
  running_.store(false);
  session_.stop();
  if (mmcss != nullptr) {
    AvRevertMmThreadCharacteristics(mmcss);
  }
}

}
