#include "server.h"

#include <boost/lexical_cast.hpp>
#include <memory>

#include "logger.h"
#include "network_manager.h"
#include "network_utility.h"
#include "room_manager.h"
#include "session.h"
#include "simple_message.h"
#include "thread_pool_manager.h"
#include "user_manager.h"
#include "utility.h"

namespace {

const int MONITORING_INTERVAL_SEC = 10;

void PrintStatus() {
  auto status = ThreadPoolManager::GetInstance()->GetStatus();
  uint64_t tps = status.counter / MONITORING_INTERVAL_SEC;

  std::string profiling{"\n''''''''''''''''''''''''''''''''''''''''''''\n"};
  profiling += "running time : " +
               boost::lexical_cast<std::string>(status.running_time) + "s\n";
  profiling += "max execution time : " +
               boost::lexical_cast<std::string>(status.max_execution_ts) +
               "ms\n";
  profiling += "min execution time : " +
               boost::lexical_cast<std::string>(status.min_execution_ts) +
               "ms\n";
  profiling += "avr execution time : " +
               boost::lexical_cast<std::string>(status.avr_execution_ts) +
               "ms\n";
  profiling += "tps : " + boost::lexical_cast<std::string>(tps) + '\n';
  profiling +=
      "current user count : " +
      boost::lexical_cast<std::string>(Connection::GetCunnectionCount()) + '\n';
  profiling += "remain event count : " +
               boost::lexical_cast<std::string>(status.queue_size) + '\n';
  profiling += "............................................\n";

  Log::Print(Log::Level::INFO, profiling);
}

void RunThread() {
  try {
    while (true) {
      boost::this_thread::interruption_point();
      PrintStatus();
      std::this_thread::sleep_for(
          std::chrono::seconds(MONITORING_INTERVAL_SEC));
    }
  } catch (...) {
    Log::Print(Log::Level::INFO, "TimerEventManager Interrupted Thread");
  }
}

}  // namespace

Server::Server() {}

Server ::~Server() {
  Stop();
  Log::Print(Log::Level::INFO, "Destroy ServerApp...");
}

void Server::Start() {
  Initialize();
  thread_ = std::make_unique<boost::thread>(RunThread);
}

void Server::Stop() {
  ThreadPoolManager::GetInstance()->Stop();

  if (!!thread_ and thread_->joinable()) {
    thread_->interrupt();
    thread_->join();
  }
  thread_.reset();
}

void Server::Initialize() {
  Log::SetLogLevel(Log::Level::INFO);

  NetworkManager::GetInstance()->Start(ASIO_THREAD_COUNT);

  UserActor::InitializeHandler();

  RoomManager::CreateRoom();
}