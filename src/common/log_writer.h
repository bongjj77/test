#pragma once
#include <cstdarg>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#define MAX_LOG_TEXT_SIZE (4096)

//------------------------------ log define ------------------------------
#define LOG_WRITE(...) LogWriter::GetInstance()->Print(__VA_ARGS__)
#define LOG_DEBUG(...) LogWriter::GetInstance()->DebugPrint(__VA_ARGS__)
#define LOG_INFO(...) LogWriter::GetInstance()->InfoPrint(__VA_ARGS__)
#define LOG_WARN(...) LogWriter::GetInstance()->WarningPrint(__VA_ARGS__)
#define LOG_ERROR(...) LogWriter::GetInstance()->ErrorPrint(__VA_ARGS__)

enum class LogLevel { Debug, Info, Warning, Error, Custom };

class LogWriter {
public:
  static LogWriter *GetInstance();
  bool LogInit(const std::string &fileName, int backupHour, LogLevel level);
  void Print(const char *format, ...);
  void DebugPrint(const char *format, ...);
  void ErrorPrint(const char *format, ...);
  void WarningPrint(const char *format, ...);
  void InfoPrint(const char *format, ...);

private:
  LogWriter();
  ~LogWriter();
  bool Open(const std::string &fileName);
  void WriteLog(LogLevel type, const char *format, va_list args);
  bool SetDailyBackupTime(int hour);

  std::ofstream _logStream;
  std::string _path;
  LogLevel _level = LogLevel::Info;
  int _dailyBackupHour = -1;
  int _lastBackupDay = -1;
  std::mutex _logLock;
  bool _monitorLock = false;
  bool _fileLock = false;
  std::vector<std::string> _logTypeList = {"DEBUG", "INFO", "WARNING", "ERROR", "CUSTOM"};
};
