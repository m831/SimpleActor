#pragma once

#include <boost/thread.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

#include "concurrent_queue.h"
#include "utility.h"

//:
//: monitoring status ±¸Á¶Ã¼
//:
struct ThreadStatus {
  ThreadStatus(uint64_t rt, uint64_t cnt, uint64_t max, uint64_t min,
               double avr, size_t qs)
      : running_time(rt),
        counter(cnt),
        max_execution_ts(max),
        min_execution_ts(min),
        avr_execution_ts(avr),
        queue_size(qs) {}
  int64_t running_time;
  uint64_t counter;
  uint64_t max_execution_ts;
  uint64_t min_execution_ts;
  double avr_execution_ts;
  size_t queue_size;
};

//:
//: ConcurrentQueue + Thread Pool
//:
class ThreadPoolManager {
#pragma region singleton pattern
 public:
  static std::shared_ptr<ThreadPoolManager>& GetInstance() {
    std::call_once(ThreadPoolManager::once_flag_, []() {
      instance_.reset(new ThreadPoolManager());
      instance_->Initialize(THREAD_POOL_THREAD_COUNT);
    });
    return instance_;
  }

  ~ThreadPoolManager();

  void Initialize(const size_t thread_count);

 private:
  ThreadPoolManager();

  static std::once_flag once_flag_;
  static std::shared_ptr<ThreadPoolManager> instance_;
#pragma endregion

 public:
  void Stop();
  void PushTask(const std::function<void()>& task);

  ThreadStatus GetStatus();

 private:
  ConcurrentQueue<std::function<void()>> task_queue_;
  std::vector<std::unique_ptr<boost::thread>> threads_;

  void RunThread();
};
