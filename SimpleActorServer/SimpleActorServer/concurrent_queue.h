#pragma once

#include <concurrent_queue.h>

#include <atomic>

#include "utility.h"

//:
//: Thread Safe Queue
//: thread safe한 Size 함수 제공
//:
template <typename T>
class ConcurrentQueue {
 public:
  ConcurrentQueue() : count_(0) {}
  ~ConcurrentQueue() {
    T tmp;
    while (not queue_.empty()) {
      queue_.try_pop(tmp);
    }
  }

  void Enqueue(const T& value);
  bool TryDequeue(T* value);
  bool IsEmpty();
  size_t Size();

 private:
  std::atomic<size_t> count_;
  concurrency::concurrent_queue<T> queue_;
};

template <typename T>
void ConcurrentQueue<T>::Enqueue(const T& value) {
  queue_.push(value);
  count_.fetch_add(1);
}

template <typename T>
bool ConcurrentQueue<T>::TryDequeue(T* retVal) {
  if (queue_.try_pop(*retVal)) {
    count_.fetch_sub(1);
    return true;
  }
  return false;
}

template <typename T>
bool ConcurrentQueue<T>::IsEmpty() {
  return queue_.empty();
}

template <typename T>
size_t ConcurrentQueue<T>::Size() {
  return count_.load();
}