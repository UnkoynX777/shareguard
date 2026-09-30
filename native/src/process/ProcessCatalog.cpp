#include "process/ProcessCatalog.hpp"

#include "platform/windows/ProcessApi.hpp"
#include "process/AudioSessionCatalog.hpp"

namespace shareguard {

ProcessSnapshot ProcessCatalog::snapshot(bool includeBackground) const {
  const std::vector<RawProcess> processes = enumerateProcesses();
  ProcessSnapshot snapshot;
  snapshot.parents.reserve(processes.size());
  for (const RawProcess& process : processes) {
    snapshot.parents.emplace(process.pid, process.parentPid);
  }
  AudioSessionCatalog sessions;
  snapshot.groups =
      groupProcesses(processes, sessions.activeProcessIds(), visibleWindowProcessIds(), includeBackground);
  return snapshot;
}

}
