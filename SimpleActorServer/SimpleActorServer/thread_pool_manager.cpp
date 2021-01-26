#include "thread_pool_manager.h"

#include <boost/atomic.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/lock_guard.hpp>
#include <mutex>
#include <unordered_map>

#include "logger.h"

namespace {

boost::detail::spinlock profiling_lock;
const int64_t start_ts = Utility::GetMillisTimestampFromNow();
uint64_t counter = 0;
uint64_t max_execution_ts = 0;
uint64_t min_execution_ts = UINT64_MAX;
double avr_execution_ts = 0.0f;

void UpdateStatus(const uint64_t ts) {
  boost::lock_guard<boost::detail::spinlock> lock{profiling_lock};
  ++counter;
  max_execution_ts = std::max<uint64_t>(ts, max_execution_ts);
  min_execution_ts = std::min<uint64_t>(ts, min_execution_ts);

  const double old_weight = (counter - 1) / (double)counter;
  const double new_weight = 1 / (double)counter;
  avr_execution_ts = (avr_execution_ts * old_weight) + (ts * new_weight);
}

}  // namespace

std::once_flag ThreadPoolManager::once_flag_;
std::shared_ptr<ThreadPoolManager> ThreadPoolManager::instance_ = nullptr;

ThreadPoolManager::ThreadPoolManager() {}

ThreadPoolManager::~ThreadPoolManager() {
  Stop();
  Log::Print(Log::Level::INFO, "Destroy ThreadPoolManager...");
}

void ThreadPoolManager::Initialize(const size_t thread_count) {
  if (not threads_.empty()) {
    return;
  }

  threads_.reserve(thread_count);
  for (size_t i = 0; i < thread_count; ++i) {
    threads_.emplace_back(
        std::make_unique<boost::thread>([this]() { this->RunThread(); }));
  }
}

void ThreadPoolManager::Stop() {
  for (size_t i = 0; i < threads_.size(); ++i) {
    auto& thread = threads_[i];
    if (!!thread and thread->joinable()) {
      thread->interrupt();
      thread->join();
    }
    thread.reset();
  }
  threads_.clear();
}

void ThreadPoolManager::PushTask(const std::function<void()>& task) {
  task_queue_.Enqueue(task);
}

ThreadStatus ThreadPoolManager::GetStatus() {
  boost::lock_guard<boost::detail::spinlock> lock{profiling_lock};
  const int64_t running_time = Utility::GetMillisTimestampFromNow() - start_ts;
  const int64_t tps = counter;
  counter = 0;
  return ThreadStatus(running_time, tps, max_execution_ts, min_execution_ts,
                      avr_execution_ts, task_queue_.Size());
}

void ThreadPoolManager::RunThread() {
  try {
    std::function<void()> task = nullptr;

    while (true) {
      boost::this_thread::interruption_point();
      if (task_queue_.TryDequeue(&task)) {
        uint64_t pre_ts = Utility::GetMillisTimestampFromNow();
        task();
        uint64_t now_ts = Utility::GetMillisTimestampFromNow();
        UpdateStatus(now_ts - pre_ts);
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }
  } catch (...) {
    Log::Print(Log::Level::INFO, "ThreadPoolManager Interrupted Thread");
  }
}