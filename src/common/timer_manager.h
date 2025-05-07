#pragma once
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // 최소한의 헤더 포함
#include <windows.h>
#include <winsock2.h>
#else
#include <sys/time.h>
#endif

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <thread>

class TimerManager {
public:
  using TimerCallback = std::function<void()>;

  TimerManager();
  ~TimerManager();

  int AddTimer(int intervalMilliseconds, TimerCallback callback);
  void RemoveTimer(int timerId);
  void Clear();

private:
  void TimerThread();

  struct TimerInfo {
    int interval;
    TimerCallback callback;
    std::chrono::steady_clock::time_point nextCallTime;
  };

  std::map<int, TimerInfo> _timers;
  std::atomic<bool> _running{true};
  std::thread _thread;
  std::mutex _mutex;
  std::condition_variable _condition;
  int _nextId{1};
};
