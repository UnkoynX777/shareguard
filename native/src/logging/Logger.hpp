#pragma once

#include <string>

namespace shareguard {

class Logger {
 public:
  static void setDebug(bool enabled);
  static void info(const std::string& message);
  static void error(const std::string& message);

 private:
  static void write(const std::string& message, bool always);
};

}
