#include "config_parser.h"
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#else
#include <cstdlib>
#endif
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

std::shared_ptr<ConfigParser> ConfigParser::instance = nullptr;

std::shared_ptr<ConfigParser> ConfigParser::GetInstance() {
  if (instance == nullptr) {
    instance = std::make_shared<ConfigParser>();
  }
  return instance;
}

ConfigParser::ConfigParser() {}
ConfigParser::~ConfigParser() {}

std::string ConfigParser::Trim(const std::string &str) {
  if (str.empty())
    return str;

  auto start = str.begin();
  while (start != str.end() && std::isspace(*start)) {
    ++start;
  }

  auto end = str.end();
  do {
    --end;
  } while (std::distance(start, end) > 0 && std::isspace(*end));

  return std::string(start, end + 1);
}

std::pair<std::string, std::string> ConfigParser::LineParse(const std::string &line) {
  if (line.empty()) {
    return {"", ""};
  }

  // '#' 뒤는 주석으로 간주하고 잘라냄
  std::string trimmedLine = line.substr(0, line.find('#'));
  trimmedLine = Trim(trimmedLine);

  if (trimmedLine.empty()) {
    return {"", ""}; // 주석이나 빈 라인은 무시
  }

  auto delimiterPos = trimmedLine.find('=');
  if (delimiterPos == std::string::npos) {
    return {"", ""}; // '='가 없다면 잘못된 형식이므로 무시
  }

  std::string name = trimmedLine.substr(0, delimiterPos);
  std::string value = trimmedLine.substr(delimiterPos + 1);

  name = Trim(name);
  value = Trim(value);

  return {name, value};
}

bool ConfigParser::LoadFile(const std::string &filePath) {
  _configs = _defaultConfigList;

  if (filePath.empty() || !std::filesystem::exists(filePath)) {
    return false;
  }

  std::ifstream fileStream(filePath);
  if (!fileStream) {
    return false;
  }

  std::string line;
  while (std::getline(fileStream, line)) {
    auto [name, value] = LineParse(line);
    if (!name.empty()) {
      std::string upperName = name;
      std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
      _configs[upperName] = value;
    }
  }

  _filePath = filePath;
  return true;
}

bool ConfigParser::IsExist(const std::string &name) const { return _configs.count(name) != 0; }

//====================================================================================================
// GetValue
// 1. 환경변수
// 2. 기본값
// 3. 기본 설정 값
//====================================================================================================
std::string ConfigParser::GetValue(const std::string &name, const std::string &defulat /* = "" */) {
  std::string upperName = name;
  std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);

  // 먼저 환경 변수에서 값을 찾음
  std::string value = defulat;
#ifdef _WIN32
  char buffer[256];
  if (GetEnvironmentVariable(upperName.c_str(), buffer, 256)) {
    value = buffer;
  }
#else
  const char *envValue = std::getenv(upperName.c_str());
  if (envValue != nullptr) {
    value = envValue;
  }
#endif

  // 환경 변수에 없으면 _configs에서 찾음
  if (value.empty()) {
    if (auto it = _configs.find(upperName); it != _configs.end()) {
      value = it->second;
    }
  } else {
    _configs[name] = value;
  }

  return value;
}

bool ConfigParser::Reload() { return LoadFile(_filePath); }

void ConfigParser::SetDefaultValue(const std::string &name, const std::string &value) {
  std::string upperName = name;
  std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
  _defaultConfigList[upperName] = value;
}

void ConfigParser::SetValue(const std::string &name, const std::string &value) { _configs[name] = value; }
