#include "timer_manager.h"

TimerManager::TimerManager() { _thread = std::thread(&TimerManager::TimerThread, this); }

TimerManager::~TimerManager() {
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _running = false;
  }
  _condition.notify_all();
  if (_thread.joinable()) {
    _thread.join();
  }
  Clear();
}

int TimerManager::AddTimer(int intervalMilliseconds, TimerCallback callback) {
  std::lock_guard<std::mutex> lock(_mutex);
  int id = _nextId++;
  TimerInfo info{intervalMilliseconds, callback,
                 std::chrono::steady_clock::now() + std::chrono::milliseconds(intervalMilliseconds)};
  _timers[id] = info;
  _condition.notify_all();
  return id;
}

void TimerManager::RemoveTimer(int timerId) {
  std::lock_guard<std::mutex> lock(_mutex);
  _timers.erase(timerId);
}

void TimerManager::Clear() {
  std::lock_guard<std::mutex> lock(_mutex);
  _timers.clear();
}

void TimerManager::TimerThread() {
  std::unique_lock<std::mutex> lock(_mutex);
  while (_running) {
    if (_timers.empty()) {
      _condition.wait(lock, [this] { return !_running || !_timers.empty(); });
    } else {
      auto nextCallTime = std::min_element(_timers.begin(), _timers.end(), [](const auto &lhs, const auto &rhs) {
                            return lhs.second.nextCallTime < rhs.second.nextCallTime;
                          })->second.nextCallTime;

      if (_condition.wait_until(lock, nextCallTime, [this] { return !_running; })) {
        return;
      }

      auto now = std::chrono::steady_clock::now();
      for (auto &[id, timer] : _timers) {
        if (now >= timer.nextCallTime) {
          timer.callback();
          timer.nextCallTime = now + std::chrono::milliseconds(timer.interval);
        }
      }
    }
  }
}
