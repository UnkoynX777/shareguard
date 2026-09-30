#pragma once

#include "process/ProcessModel.hpp"

namespace shareguard {

class ProcessCatalog {
 public:
  ProcessSnapshot snapshot(bool includeBackground) const;
};

}
