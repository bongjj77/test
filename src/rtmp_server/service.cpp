#include "main_object.h"
#include <condition_variable>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <signal.h>
#include <thread>
#include <utility>

#define _PROGRAM_NAME_ "rtmp-server"

std::condition_variable closeWait;
std::mutex closeMutex;
bool closeFlag = false;

// Signal handler
void SignalHandler(int sig) {
  std::lock_guard<std::mutex> lock(closeMutex);
  closeFlag = true;
  closeWait.notify_all();
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
}

//===============================================================================================
// 설정 초기화
//===============================================================================================
std::shared_ptr<ConfigParser> InitConfig(int argc, char *argv[]) {
  auto config = ConfigParser::GetInstance();
  std::string configPath = ".env";

  // Parse command line arguments
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "-c" && i + 1 < argc) {
      configPath = argv[++i];
    }
  }

  if (!config->LoadFile(configPath)) {
    std::cerr << "Failed to load config file, using default values." << std::endl;
  }

  return config;
}

//===============================================================================================
// 로그 초기화
//===============================================================================================
bool InitLog(const std::string &path, const std::string &backupHour, const std::string &level) {
  std::filesystem::path logPath(path);
  if (!std::filesystem::exists(logPath.parent_path())) {
    std::filesystem::create_directories(logPath.parent_path());
  }

  std::ofstream logFile(path, std::ios::app);
  if (!logFile.is_open()) {
    std::cerr << "Failed to open log file: " << path << std::endl;
    return false;
  }

  if (!LogWriter::GetInstance()->LogInit(path, std::stoi(backupHour), static_cast<LogLevel>(std::stoi(level)))) {
    fprintf(stderr, "%s log system error \n", _PROGRAM_NAME_);
    return false;
  }

  return true;
}

//===============================================================================================
// Main
//===============================================================================================
int main(int argc, char *argv[]) {
  // 설정 초기화 및 로그 출력
  auto config = InitConfig(argc, argv);

  // 로그 초기화
  if (!InitLog(config->GetValue("LOG_FILE_PATH", "./log/rtmp_server.log"), config->GetValue("SYS_LOG_BACKUP_HOUR", "4"),
               config->GetValue("LOG_LEVEL", "0"))) {
    return -1;
  }

  // Create main object
  auto param = std::make_unique<Config>();
  param->version = config->GetValue("VERSION", _PROGRAM_NAME_);
  param->hostIp = config->GetValue("HOST_IP", Network::TcpManager::GetLocalIP());
  param->studioPort = std::stoi(config->GetValue("STUDIO_PORT", "1935"));
  param->playerPort = std::stoi(config->GetValue("PLAYER_PORT", "7775"));
  param->controllerHost = config->GetValue("CONTROLLER_HOST", "localhost");
  param->controllerPort = std::stoi(config->GetValue("CONTROLLER_PORT", "7777"));

  // Config 정보 출력
  std::cout << "[ Configuration Settings ]" << std::endl;
  std::cout << param->ToString();
  std::cout << "[ " << _PROGRAM_NAME_ << " start(" << GetStringTime(0) << ")]\n" << std::endl;

  auto mainObject = std::make_shared<MainObject>();
  if (!mainObject->Create(std::move(param))) {
    LOG_WRITE("[ %s ] Main object create error", _PROGRAM_NAME_);
    return -1;
  }

  // Set up signal handler
  signal(SIGINT, SignalHandler);
  signal(SIGTERM, SignalHandler);

  // Wait for program termination signal
  std::unique_lock<std::mutex> lock(closeMutex);
  closeWait.wait(lock, [] { return closeFlag; });

  // Release resources
  mainObject->Release();
  mainObject = nullptr;

  // Log program end
  LOG_WRITE("********** [ %s end(%s) ]**********", _PROGRAM_NAME_, GetStringTime(0).c_str());

  return 0;
}
