#include "logger.h"

#include <Windows.h>

#include <iostream>
#include <thread>

namespace {

static Log::Level LOG_LEVEL = Log::Level::DEBUG;

};

void Log::SetLogLevel(const Level level) { LOG_LEVEL = level; }

void Log::Print(const Level level, const std::string& v) {
  if (level < LOG_LEVEL) {
    return;
  }
  SYSTEMTIME cur_time;
  GetLocalTime(&cur_time);
  std::cout << "[" << cur_time.wYear << "-" << cur_time.wMonth << "-"
            << cur_time.wDay << " " << cur_time.wHour << ":" << cur_time.wMinute
            << ":" << cur_time.wSecond << " " << cur_time.wMilliseconds << "] ["
            << std::this_thread::get_id() << "]\t" << v << std::endl;
}