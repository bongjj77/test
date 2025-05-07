#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

class ConfigParser {
public:
  ConfigParser();
  ~ConfigParser();

  static std::shared_ptr<ConfigParser> GetInstance();
  bool LoadFile(const std::string &filePath);
  bool Reload();
  std::string GetValue(const std::string &name, const std::string &defulat = "");
  bool IsExist(const std::string &name) const;
  void SetDefaultValue(const std::string &name, const std::string &value);
  void SetValue(const std::string &name, const std::string &value);

private:
  std::unordered_map<std::string, std::string> _configs;
  std::unordered_map<std::string, std::string> _defaultConfigList;
  std::string _filePath;

  static std::string Trim(const std::string &str);
  static std::pair<std::string, std::string> LineParse(const std::string &line);

  static std::shared_ptr<ConfigParser> instance;
};
