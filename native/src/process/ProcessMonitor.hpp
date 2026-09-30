#pragma once

#include "process/ProcessCatalog.hpp"

#include <atomic>
#include <functional>
#include <thread>

namespace shareguard {

class ProcessMonitor {
 public:
  using Listener = std::function<void(const ProcessSnapshot&)>;

  ProcessMonitor(ProcessCatalog& catalog, Listener listener);
  ~ProcessMonitor();

  ProcessMonitor(const ProcessMonitor&) = delete;
  ProcessMonitor& operator=(const ProcessMonitor&) = delete;

  void setIncludeBackground(bool includeBackground);

 private:
  void loop(std::stop_token stop);
  std::string signature(const std::vector<ApplicationGroup>& groups) const;

  ProcessCatalog& catalog_;
  Listener listener_;
  std::atomic<bool> includeBackground_{false};
  std::jthread thread_;
};

}
