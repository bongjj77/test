#include "log_writer.h"
#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>

LogWriter *LogWriter::GetInstance() {
  static LogWriter instance;
  return &instance;
}

bool LogWriter::LogInit(const std::string &filePath, int backupHour, LogLevel level) {
  if (Open(filePath)) {
    SetDailyBackupTime(backupHour);
    _level = level;

    std::cout << std::endl << "[ Log ]" << std::endl;
    std::cout << "  - Path : " << filePath << std::endl;
    std::cout << "  - Backup hour : " << backupHour << std::endl;
    std::cout << "  - Level : " << _logTypeList[static_cast<int>(level)] << std::endl << std::endl;
    return true;
  }
  return false;
}

void LogWriter::Print(const char *format, ...) {
  va_list args;
  va_start(args, format);
  WriteLog(LogLevel::Custom, format, args);
  va_end(args);
}

void LogWriter::DebugPrint(const char *format, ...) {
  va_list args;
  va_start(args, format);
  WriteLog(LogLevel::Debug, format, args);
  va_end(args);
}

void LogWriter::ErrorPrint(const char *format, ...) {
  va_list args;
  va_start(args, format);
  WriteLog(LogLevel::Error, format, args);
  va_end(args);
}

void LogWriter::WarningPrint(const char *format, ...) {
  va_list args;
  va_start(args, format);
  WriteLog(LogLevel::Warning, format, args);
  va_end(args);
}

void LogWriter::InfoPrint(const char *format, ...) {
  va_list args;
  va_start(args, format);
  WriteLog(LogLevel::Info, format, args);
  va_end(args);
}

LogWriter::LogWriter() {}

LogWriter::~LogWriter() {
  if (_logStream.is_open()) {
    _logStream.close();
  }
}

bool LogWriter::Open(const std::string &filePath) {
  if (filePath.empty()) {
    return false;
  }

  _path = filePath;

  if (_logStream.is_open()) {
    _logStream.close();
  }

  _logStream.open(_path, std::fstream::out | std::fstream::app);
  return _logStream.is_open();
}

void LogWriter::WriteLog(LogLevel type, const char *format, va_list args) {
  if (type < _level) {
    return;
  }

  auto now = std::chrono::system_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  auto now_c = std::chrono::system_clock::to_time_t(now);
  std::tm tm;
#ifdef _WIN32
  localtime_s(&tm, &now_c);
#else
  localtime_r(&now_c, &tm);
#endif

  char timestamp[64];
  std::snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d.%03d", tm.tm_year + 1900, tm.tm_mon + 1,
                tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, static_cast<int>(ms.count()));

  char buffer[MAX_LOG_TEXT_SIZE];
  std::vsprintf(buffer, format, args);

  std::string logLine = "[" + _logTypeList[static_cast<int>(type)] + "] " + timestamp + " " + buffer;

  if (!_monitorLock) {
    std::cout << logLine << std::endl;
  }

  std::lock_guard<std::mutex> lock(_logLock);

  if (_dailyBackupHour != -1 && _lastBackupDay != tm.tm_mday && tm.tm_hour >= _dailyBackupHour) {
    if (_logStream.is_open()) {
      _logStream.close();
    }

    std::ostringstream backupFileName;
    backupFileName << _path << "_" << (tm.tm_year + 1900) << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1)
                   << std::setw(2) << std::setfill('0') << tm.tm_mday << "_logbackup";
    std::rename(_path.c_str(), backupFileName.str().c_str());

    _lastBackupDay = tm.tm_mday;
    Open(_path);
  }

  if (_logStream.is_open() && !_fileLock) {
    _logStream << logLine << std::endl;
  }
}

bool LogWriter::SetDailyBackupTime(int hour) {
  if (hour < 0 || hour > 23) {
    _dailyBackupHour = -1;
    _lastBackupDay = -1;
  } else {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &now_c);
#else
    localtime_r(&now_c, &tm);
#endif
    _lastBackupDay = tm.tm_mday;
    _dailyBackupHour = hour;
  }

  return _logStream.is_open();
}
