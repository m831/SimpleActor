#include "timer_event_manager.h"

#include "actor_base_model.h"
#include "logger.h"
#include "utility.h"

TimerEventWorker::TimerEventWorker()
    : thread_(new boost::thread([this]() { this->RunThread(); })) {}

TimerEventWorker::~TimerEventWorker() {
  if (!!thread_ and thread_->joinable()) {
    thread_->interrupt();
    thread_->join();
  }
  thread_.reset();
}

void TimerEventWorker::RunThread() {
  try {
    while (true) {
      boost::this_thread::interruption_point();
      MakePendingTimerEvent();
      ExecuteTimerEvent();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  } catch (...) {
    Log::Print(Log::Level::INFO, "TimerEventManager Interrupted Thread");
  }
}

void TimerEventWorker::MakePendingTimerEvent() {
  while (true) {
    TimerEventInfo info;
    if (not TimerEventManager::GetInstance()->GetEventInfo(&info)) {
      return;
    }

    auto iter = event_queue_by_timestamp_.find(info.expected_msec);
    if (iter == event_queue_by_timestamp_.end()) {
      std::queue<TimerEventInfo> q;
      q.emplace(info);
      event_queue_by_timestamp_.emplace(info.expected_msec, q);
    } else {
      iter->second.emplace(info);
    }
  }
}

void TimerEventWorker::ExecuteTimerEvent() {
  const int64_t now_msec = Utility::GetMillisTimestampFromNow();

  std::vector<int64_t> removal_keys;
  removal_keys.reserve(event_queue_by_timestamp_.size());

  for (auto& entry : event_queue_by_timestamp_) {
    const int64_t expected_msec = entry.first;
    if (now_msec < expected_msec) {
      continue;
    }

    auto& queue = entry.second;
    while (not queue.empty()) {
      auto& info = queue.front();
      info.actor->AsyncTask(info.task);
      queue.pop();
    }

    removal_keys.emplace_back(expected_msec);
  }

  for (const int64_t key : removal_keys) {
    event_queue_by_timestamp_.erase(key);
  }
}

//----------------------------------------------------------------------

std::once_flag TimerEventManager::once_flag_;
std::shared_ptr<TimerEventManager> TimerEventManager::instance_ = nullptr;

TimerEventManager::TimerEventManager() {}

TimerEventManager::~TimerEventManager() {
  for (auto& worker : timer_event_workers_) {
    if (not worker) {
      continue;
    }
    worker.reset();
  }
}

void TimerEventManager::Initialize(const size_t thread_count) {
  timer_event_workers_.reserve(thread_count);
  for (size_t i = 0; i < thread_count; ++i) {
    timer_event_workers_.emplace_back(std::make_unique<TimerEventWorker>());
  }
}

void TimerEventManager::InvokeEvent(
    const std::shared_ptr<ActorBaseModel>& actor,
    const std::function<void()>& task, const int64_t delay_msec) {
  if (delay_msec <= 0) {
    actor->AsyncTask(task);
    return;
  }

  TimerEventInfo info;
  info.actor = actor;
  info.task = task;
  info.expected_msec = Utility::GetMillisTimestampFromNow() + delay_msec;

  event_queue_.Enqueue(info);
}

bool TimerEventManager::GetEventInfo(TimerEventInfo* info) {
  return event_queue_.TryDequeue(info);
}