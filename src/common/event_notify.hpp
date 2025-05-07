#pragma once
#include <condition_variable>
#include <mutex>

class EventNotify {
public:
  // Notifies one waiting thread.
  void Notify() {
    std::lock_guard<std::mutex> lock(_mutex);
    ++_count;
    _condition.notify_one();
  }

  // Blocks the current thread until notified.
  void Wait() {
    std::unique_lock<std::mutex> lock(_mutex);
    _condition.wait(lock, [this] { return _count > 0; });
    --_count;
  }

  // Tries to wait and returns immediately if not notified.
  bool TryWait() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_count > 0) {
      --_count;
      return true;
    }
    return false;
  }

private:
  std::mutex _mutex;
  std::condition_variable _condition;
  int _count = 0;
};
