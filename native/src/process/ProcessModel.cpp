#include "process/ProcessModel.hpp"

namespace shareguard {

const char* captureStrategyName(CaptureStrategy strategy) {
  switch (strategy) {
    case CaptureStrategy::SingleProcessExclusion:
      return "SingleProcessExclusion";
    case CaptureStrategy::AllowedProcessMix:
      return "AllowedProcessMix";
    default:
      return "SystemLoopback";
  }
}

}
