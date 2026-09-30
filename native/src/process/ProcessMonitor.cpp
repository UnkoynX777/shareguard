#include "process/ProcessMonitor.hpp"

#include "platform/windows/ComRuntime.hpp"

#include <chrono>
#include <sstream>

namespace shareguard {

ProcessMonitor::ProcessMonitor(ProcessCatalog& catalog, Listener listener)
    : catalog_(catalog), listener_(std::move(listener)), thread_([this](std::stop_token stop) { loop(stop); }) {}

ProcessMonitor::~ProcessMonitor() {
  if (thread_.joinable()) {
    thread_.request_stop();
  }
}

void ProcessMonitor::setIncludeBackground(bool includeBackground) { includeBackground_.store(includeBackground); }

void ProcessMonitor::loop(std::stop_token stop) {
  ComRuntime com;
  std::string previous;
  while (!stop.stop_requested()) {
    const ProcessSnapshot snapshot = catalog_.snapshot(includeBackground_.load());
    const std::string next = signature(snapshot.groups);
    if (next != previous) {
      previous = next;
      if (listener_) {
        listener_(snapshot);
      }
    }
    for (int tick = 0; tick < 10 && !stop.stop_requested(); ++tick) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}

std::string ProcessMonitor::signature(const std::vector<ApplicationGroup>& groups) const {
  std::ostringstream stream;
  for (const ApplicationGroup& group : groups) {
    stream << group.id << '|' << group.displayName << '|' << (group.audioActive ? '1' : '0') << '|';
    for (const std::uint32_t root : group.rootPids) {
      stream << root << ',';
    }
    stream << ';';
  }
  return stream.str();
}

}
