#include "system_monitor.h"

//====================================================================================================
// SystemMonitor
//====================================================================================================
SystemMonitor::SystemMonitor() {
#ifdef _WIN32
  _initialized = false;
#endif
}

//====================================================================================================
// ~SystemMonitor
//====================================================================================================
SystemMonitor::~SystemMonitor() { Close(); }

//====================================================================================================
// Create
//====================================================================================================
bool SystemMonitor::Create() {
  _closeFlag = false;

#ifndef _WIN32
  _statFile.open("/proc/stat");
  if (!_statFile.is_open()) {
    LOG_ERROR("SystemMonitor /proc/stat open fail");
    return false;
  }
#endif

  _thread = std::thread([this]() {
    while (!_closeFlag.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      UpdateInfo();
    }
  });
  return true;
}

//====================================================================================================
// Close
//====================================================================================================
void SystemMonitor::Close() {
  if (_thread.joinable()) {
    _closeFlag.store(true);
    _thread.join();
  }
#ifndef _WIN32
  if (_statFile.is_open()) {
    _statFile.close();
  }
#endif
}

//====================================================================================================
// UpdateInfo
//====================================================================================================
void SystemMonitor::UpdateInfo() {
#ifdef _WIN32
  if (!_initialized) {
    PdhOpenQuery(nullptr, 0, &_cpuQuery);
    PdhAddCounter(_cpuQuery, "\\Processor(_Total)\\% Processor Time", 0, &_cpuTotal);
    PdhCollectQueryData(_cpuQuery);
    _initialized = true;
  }

  PDH_FMT_COUNTERVALUE counterVal;
  PdhCollectQueryData(_cpuQuery);
  PdhGetFormattedCounterValue(_cpuTotal, PDH_FMT_DOUBLE, nullptr, &counterVal);
  _resInfo.cpuUsed = static_cast<int>(counterVal.doubleValue);

  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&memInfo);
  _resInfo.memTotal = static_cast<int>(memInfo.ullTotalPhys / 1024 / 1024);
  _resInfo.memUsed = static_cast<int>((memInfo.ullTotalPhys - memInfo.ullAvailPhys) / 1024 / 1024);

#else
  uint32_t sum = 0, idle = 0;
  if (_statFile.is_open()) {
    _statFile.seekg(0, std::ios::beg);
    char buffer[MAX_READ_SIZE];
    if (_statFile.getline(buffer, sizeof(buffer))) {
      char cpu[5];
      uint32_t user, nice, system, iowait, irq, softirq, steal;
      sscanf(buffer, "%s %u %u %u %u %u %u %u", cpu, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
      sum = user + nice + system + idle + iowait + irq + softirq + steal;
      _resInfo.cpuUsed = (1000 * (sum - _lastSum - (idle - _lastIdle)) / (sum - _lastSum) + 5) / 10;
      _lastIdle = idle;
      _lastSum = sum;
    } else {
      LOG_ERROR("SystemMonitor /proc/stat read fail");
    }
  }

  struct sysinfo sysInfo;
  if (sysinfo(&sysInfo) == 0) {
    _resInfo.memTotal = static_cast<int>(sysInfo.totalram / 1024 / 1024);
    _resInfo.memUsed = static_cast<int>((sysInfo.totalram - sysInfo.freeram) / 1024 / 1024);
  }
#endif
}
