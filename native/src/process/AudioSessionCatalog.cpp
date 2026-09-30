#include "process/AudioSessionCatalog.hpp"

#include "platform/windows/ProcessApi.hpp"

namespace shareguard {

std::unordered_set<std::uint32_t> AudioSessionCatalog::activeProcessIds() const { return activeAudioProcessIds(); }

}
