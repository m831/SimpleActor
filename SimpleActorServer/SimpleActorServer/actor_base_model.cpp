#include "actor_base_model.h"

#include "thread_pool_manager.h"
#include "timer_event_manager.h"
#include "utility.h"

ActorBaseModel::ActorBaseModel()
    : recursive_interval_msec_(0), task_count_(0) {}

ActorBaseModel::~ActorBaseModel() {
  std::function<void()> task;
  while (not task_queue_.IsEmpty()) {
    task_queue_.TryDequeue(&task);
  }
  task_count_ = 0;
}

void ActorBaseModel::StartRecursiveEvent(const int64_t inteval_msec) {
  const bool is_first = (recursive_interval_msec_ == 0);

  recursive_interval_msec_ = inteval_msec;
  const int64_t expected_msec =
      Utility::GetMillisTimestampFromNow() + recursive_interval_msec_;

  if (is_first) {
    SelfEvent(expected_msec);
  }
}

void ActorBaseModel::AsyncTask(const std::function<void()>& task) {
  task_queue_.Enqueue(task);

  if (++task_count_ == 1) {
    auto self = shared_from_this();
    ThreadPoolManager::GetInstance()->PushTask(
        [this, self]() { FlushEvent(); });
  }
}

void ActorBaseModel::AsyncTask(const std::function<void()>& task,
                               const int64_t delay_msec) {
  auto self = shared_from_this();
  TimerEventManager::GetInstance()->InvokeEvent(self, task, delay_msec);
}

void ActorBaseModel::SelfEvent(const int64_t expected_msec) {
  if (recursive_interval_msec_ == 0) {
    return;
  }

  const int64_t now_msec = Utility::GetMillisTimestampFromNow();
  int64_t delay_msec = 0;
  if (now_msec > expected_msec) {
    delay_msec = recursive_interval_msec_ - (now_msec - expected_msec);
  } else {
    delay_msec = recursive_interval_msec_;
  }

  const int64_t next_expected_msec = expected_msec + recursive_interval_msec_;
  auto self = shared_from_this();
  TimerEventManager::GetInstance()->InvokeEvent(
      self,
      [this, self, next_expected_msec]() { SelfEvent(next_expected_msec); },
      delay_msec);
}

void ActorBaseModel::FlushEvent() {
  std::function<void()> task;

  while (true) {
    if (not task_queue_.TryDequeue(&task)) {
      break;
    }
    --task_count_;
    task();
  }
}