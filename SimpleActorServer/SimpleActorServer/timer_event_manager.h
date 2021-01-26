#pragma once

#include <boost/thread.hpp>
#include <functional>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

#include "concurrent_queue.h"
#include "utility.h"

class ActorBaseModel;

//:
//: Actor와 Function을 가지는 구조체
//:
struct TimerEventInfo {
  std::shared_ptr<ActorBaseModel> actor;
  std::function<void()> task;
  int64_t expected_msec;
};

//:
//: actor에게 task를 전달하는 worker 스레드
//: 시간 순서대로 실행을 보장
//:
class TimerEventWorker {
 public:
  TimerEventWorker();
  ~TimerEventWorker();

 private:
  using EventQeueHashMap =
      std::unordered_map<int64_t, std::queue<TimerEventInfo>>;

  EventQeueHashMap event_queue_by_timestamp_;
  std::unique_ptr<boost::thread> thread_;

  void RunThread();
  void MakePendingTimerEvent();
  void ExecuteTimerEvent();
};

//:
//: Worker의 실행을 관리하는 Manager클래스
//:
class TimerEventManager {
#pragma region singleton pattern
 public:
  static std::shared_ptr<TimerEventManager>& GetInstance() {
    std::call_once(TimerEventManager::once_flag_, []() {
      instance_.reset(new TimerEventManager());
      instance_->Initialize(TIMER_EVENT_THREAD_COUNT);
    });

    return instance_;
  }

  ~TimerEventManager();

  void Initialize(const size_t thread_count);

 private:
  TimerEventManager();
  static std::once_flag once_flag_;
  static std::shared_ptr<TimerEventManager> instance_;
#pragma endregion

 public:
  void InvokeEvent(const std::shared_ptr<ActorBaseModel>& actor,
                   const std::function<void()>& task, const int64_t delay_msec);
  bool GetEventInfo(TimerEventInfo* info);

 private:
  std::vector<std::unique_ptr<TimerEventWorker>> timer_event_workers_;
  ConcurrentQueue<TimerEventInfo> event_queue_;
};
