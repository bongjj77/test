#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>

//====================================================================================================
// FileHelper
//====================================================================================================
class FileHelper {
public:
  FileHelper() = default;

public:
  static std::string Search(const std::string &folderName, const std::string &fileName);
  static std::string SearchFromFolders(const std::vector<std::string> &folderList, const std::string &fileName);
  static std::tuple<std::string, std::string, std::string, std::string> FilePathSplit(const std::string &filePath);
  static std::string GetFolder(const std::string &filePath);
  static std::string GetName(const std::string &filePath, bool includeExt);
  static std::string GetExt(const std::string &filePath);
  static bool IsExist(const std::string &filePath);
  static uint64_t GetFileSize(std::ifstream &fileStream);
};
