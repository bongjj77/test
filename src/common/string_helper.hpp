#pragma once
#include <algorithm>
#include <cstring>
#include <memory>
#include <random>
#include <string>
#include <vector>

//====================================================================================================
// StringHelper
//====================================================================================================
class StringHelper {
public:
  StringHelper() = default;
  ~StringHelper() = default;

  //====================================================================================================
  // string 파싱
  //====================================================================================================
  static std::shared_ptr<std::vector<std::string>> Tokenize(const std::string &text, const std::string &delimiters) {
    auto tokens = std::make_shared<std::vector<std::string>>();
    std::string::size_type lastPos = text.find_first_not_of(delimiters, 0);
    std::string::size_type pos = text.find_first_of(delimiters, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos) {
      tokens->emplace_back(text.substr(lastPos, pos - lastPos));
      lastPos = text.find_first_not_of(delimiters, pos);
      pos = text.find_first_of(delimiters, lastPos);
    }

    return tokens;
  }

  //====================================================================================================
  // string 파싱(빈문자열 추출 가능)
  //====================================================================================================
  static std::shared_ptr<std::vector<std::string>> EmptyDataTokenize(const std::string &text, char delimiter) {
    auto tokens = std::make_shared<std::vector<std::string>>();
    const char *textPos = text.c_str();

    do {
      const char *begin = textPos;

      while (*textPos != delimiter && *textPos) {
        ++textPos;
      }

      tokens->emplace_back(begin, textPos);
    } while (0 != *textPos++);

    return tokens;
  }

  //====================================================================================================
  // string formatting
  //====================================================================================================
  template <typename... Args> static std::string Format(const char *format, Args... args) {
    size_t size = snprintf(nullptr, 0, format, args...);

    std::string buffer(size, '\0');
    snprintf(buffer.data(), size + 1, format, args...);

    return buffer;
  }

  //====================================================================================================
  // string trim(right + left)
  //====================================================================================================
  static std::string Trim(const std::string &input, const std::string &erase) {
    auto result = input;
    result.erase(result.find_last_not_of(erase) + 1);
    result.erase(0, result.find_first_not_of(erase));
    return result;
  }

  //====================================================================================================
  // string right trim
  //====================================================================================================
  static std::string RightTrim(const std::string &input, const std::string &erase) {
    auto result = input;
    result.erase(result.find_last_not_of(erase) + 1);
    return result;
  }

  //====================================================================================================
  // string left trim
  //====================================================================================================
  static std::string LeftTrim(const std::string &input, const std::string &erase) {
    auto result = input;
    result.erase(0, result.find_first_not_of(erase));
    return result;
  }

  //====================================================================================================
  // Replace
  //====================================================================================================
  static std::string Replace(std::string input, const char *before, const char *after) {
    std::string::size_type pos = input.find(before);
    const std::string::size_type beforeSize = strlen(before);
    const std::string::size_type afterSize = strlen(after);

    while (pos < std::string::npos) {
      input.replace(pos, beforeSize, after);
      pos = input.find(before, pos + afterSize);
    }

    return input;
  }

  //====================================================================================================
  // Random String
  //====================================================================================================
  static std::string RandomString(uint32_t size, bool numberOnly) {
    std::string text = numberOnly ? "012345678901234567890123456789012345678901234567890123456789"
                                  : "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::random_device rd;
    std::mt19937 generator(rd());
    std::shuffle(text.begin(), text.end(), generator);

    std::string result = text.substr(0, size);
    if (result[0] == '0') {
      result[0] = 'A';
    }

    return result;
  }

  //====================================================================================================
  // Random Hex String
  //====================================================================================================
  static std::string RandomHex(uint32_t size) {
    std::string text =
        "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";

    std::random_device rd;
    std::mt19937 generator(rd());
    std::shuffle(text.begin(), text.end(), generator);

    std::string result = text.substr(0, size);
    if (result[0] == '0') {
      result[0] = 'A';
    }

    return result;
  }
};
