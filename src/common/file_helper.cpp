
#include "file_helper.h"
#include <algorithm>
#include <iostream>

//====================================================================================================
// File Search
//====================================================================================================
std::string FileHelper::Search(const std::string &folderName, const std::string &fileName) {
  for (const auto &entry : std::filesystem::recursive_directory_iterator(folderName)) {
    if (!entry.is_directory() && entry.path().filename() == fileName) {
      return entry.path().string();
    }
  }
  return "";
}

//====================================================================================================
// File Search
//====================================================================================================
std::string FileHelper::SearchFromFolders(const std::vector<std::string> &folderList, const std::string &fileName) {
  for (const auto &folder : folderList) {
    std::string searchPath = Search(folder, fileName);
    if (!searchPath.empty()) {
      return searchPath;
    }
  }
  return "";
}

//====================================================================================================
// File Path Split
//====================================================================================================
std::tuple<std::string, std::string, std::string, std::string> FileHelper::FilePathSplit(const std::string &filePath) {
  std::filesystem::path path(filePath);
  std::string ext = path.extension().string();
  if (!ext.empty() && ext[0] == '.') {
    ext.erase(0, 1);
  }
  return {path.parent_path().string(), path.filename().string(), path.stem().string(), ext};
}

//====================================================================================================
// Get file folder name
//====================================================================================================
std::string FileHelper::GetFolder(const std::string &filePath) { return std::get<0>(FilePathSplit(filePath)); }

//====================================================================================================
// Get file name
//====================================================================================================
std::string FileHelper::GetName(const std::string &filePath, bool includeExt) {
  auto [folder, name, stem, ext] = FilePathSplit(filePath);
  return includeExt ? name : stem;
}

//====================================================================================================
// Get ext name
//====================================================================================================
std::string FileHelper::GetExt(const std::string &filePath) { return std::get<3>(FilePathSplit(filePath)); }

//====================================================================================================
// Exist check
//====================================================================================================
bool FileHelper::IsExist(const std::string &filePath) { return std::filesystem::exists(filePath); }

//====================================================================================================
// File size
//====================================================================================================
uint64_t FileHelper::GetFileSize(std::ifstream &fileStream) {
  if (!fileStream.is_open()) {
    return 0;
  }
  fileStream.seekg(0, std::ios::end);
  uint64_t size = fileStream.tellg();
  fileStream.seekg(0, std::ios::beg);
  return size;
}