#pragma once

#include <string>

#define not !
#define and &&
#define or ||

const int THREAD_POOL_THREAD_COUNT = 16;
const int TIMER_EVENT_THREAD_COUNT = 4;
const int ASIO_THREAD_COUNT = 2;
const short LISTEN_PORT = 5959;

namespace Utility {

std::string GenerateStringUuid();

int RandomGenerateNumber(const int start, const int end);

int64_t GetMillisTimestampFromNow();

int64_t GetTimestampFromNow();

}  // namespace Utility