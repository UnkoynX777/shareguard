#include "platform/windows/ComRuntime.hpp"

#include <objbase.h>

namespace shareguard {

ComRuntime::ComRuntime() {
  const HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  ok_ = result == S_OK || result == S_FALSE;
}

ComRuntime::~ComRuntime() {
  if (ok_) {
    CoUninitialize();
  }
}

}
