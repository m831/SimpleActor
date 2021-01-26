#pragma once

#include <string>

class Log {
 public:
  enum class Level {
    DEBUG = 0,
    INFO,
    WARNING,
    ERROR_,
    FATAL,
  };

  static void SetLogLevel(const Level level);

  static void Print(const Level level, const std::string& v);
};