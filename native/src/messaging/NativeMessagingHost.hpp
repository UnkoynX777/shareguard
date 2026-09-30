#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <string>

namespace shareguard {

class NativeMessagingHost {
 public:
  using Handler = std::function<void(const std::string&)>;

  void send(const std::string& json);
  void run(const Handler& handler);

 private:
  std::mutex writeMutex_;
  std::atomic<bool> broken_{false};
};

}
