#pragma once

#include <functional>
#include <memory>

#include "concurrent_queue.h"

//:
//: EventData 인터페이스 구조체
//:
struct IEventData {
  IEventData() = default;
  virtual ~IEventData() = default;
};

//:
//: actor base class
//:
class ActorBaseModel : public std::enable_shared_from_this<ActorBaseModel> {
 public:
  ActorBaseModel();
  virtual ~ActorBaseModel();

  void StartRecursiveEvent(const int64_t inteval_msec);
  void FlushEvent();
  void AsyncTask(const std::function<void()>& task);
  void AsyncTask(const std::function<void()>& task, const int64_t delay_msec);

 protected:
  virtual void SelfEvent(const int64_t expected_msec);
  virtual void SendAsyncEvent(const int type,
                              const std::shared_ptr<IEventData>&) = 0;

 private:
  int64_t recursive_interval_msec_;
  std::atomic<int> task_count_;
  ConcurrentQueue<std::function<void()>> task_queue_;
};