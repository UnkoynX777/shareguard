#pragma once

namespace shareguard {

class ComRuntime {
 public:
  ComRuntime();
  ~ComRuntime();

  ComRuntime(const ComRuntime&) = delete;
  ComRuntime& operator=(const ComRuntime&) = delete;

  bool ok() const { return ok_; }

 private:
  bool ok_ = false;
};

}
