#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // 최소한의 헤더 포함
#include <Pdh.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Pdh.lib")
#else
#include <sys/sysinfo.h>
#include <unistd.h>
#define MAX_READ_SIZE 100
#endif

#include "common_function.h"
#include "log_writer.h"
#include <atomic>
#include <chrono>
#include <fstream>
#include <memory>
#include <thread>
#include <vector>

struct SystemInfo {
  int cpuUsed = 0;  // %
  int memTotal = 0; // MB
  int memUsed = 0;  // MB
};

//====================================================================================================
// SystemMonitor
//====================================================================================================
class SystemMonitor {
public:
  SystemMonitor();
  ~SystemMonitor();

  bool Create();
  void Close();
  SystemInfo GetSystemInfo() const { return _resInfo; }

private:
  void UpdateInfo();

private:
  std::thread _thread;
  std::atomic<bool> _closeFlag{false};
  SystemInfo _resInfo;

#ifdef _WIN32
  PDH_HQUERY _cpuQuery;
  PDH_HCOUNTER _cpuTotal;
  bool _initialized;
#else
  uint32_t _lastSum = 0;
  uint32_t _lastIdle = 0;
  std::ifstream _statFile;
#endif
};
