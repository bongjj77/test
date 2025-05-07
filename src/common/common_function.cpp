#include "common_function.h"
#include "./string_helper.hpp"
#include <algorithm>
#include <boost/asio.hpp>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <regex>
#include <sstream>

//====================================================================================================
// 시간 문자
// - timer 0 입력시 현재시간
//====================================================================================================
std::string GetStringTime(time_t timeValue, bool isDate /*= true*/) {
  if (timeValue == 0) {
    timeValue = time(nullptr);
  }

  tm tm;

#ifdef _WIN32
  localtime_s(&tm, &timeValue);
#else
  localtime_r(&timeValue, &tm);
#endif

  std::string timeText;

  if (isDate) {
    timeText = StringHelper::Format("%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                                    tm.tm_hour, tm.tm_min, tm.tm_sec);
  } else {
    timeText = StringHelper::Format("%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
  }

  return timeText;
}

std::string HostToIp(const std::string &host) {
  boost::asio::io_service io_service;
  // Regular expression to check if the host is already an IP address
  std::regex ip_regex(R"(^\d{1,3}(\.\d{1,3}){3}$)");
  if (std::regex_match(host, ip_regex)) {
    // If the host is already an IP address, return it as is
    return host;
  }

  try {
    // Resolve the host name to an IP address
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(host, "80");
    boost::asio::ip::tcp::resolver::iterator endpoints = resolver.resolve(query);

    // Get the first resolved endpoint and return its IP address
    boost::asio::ip::tcp::endpoint endpoint = *endpoints;
    return endpoint.address().to_string();
  } catch (const std::exception &e) {
    std::cerr << "Error resolving host: " << e.what() << std::endl;
    return "";
  }
}

//====================================================================================================
// GetCurrentTick(Millisecond)
//====================================================================================================
uint64_t GetCurrentTick() {
  uint64_t tick = 0ull;
#ifdef _WIN32
  tick = GetTickCount64();
#else
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  tick = (tp.tv_sec * 1000ull) + (tp.tv_nsec / 1000ull / 1000ull);
#endif
  return tick;
}

//====================================================================================================
// current ms time(1/1000)
//====================================================================================================
uint64_t GetCurrentMs() {
  return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now())
      .time_since_epoch()
      .count();
}

std::string GetLocalHostName() {
  try {
    // Get the local host name using Boost.Asio
    std::string hostname = boost::asio::ip::host_name();
    return hostname;
  } catch (const std::exception &e) {
    std::cerr << "Error getting local host name: " << e.what() << std::endl;
    return "";
  }
}

//====================================================================================================
// ConvertTimescale
// - from_timescale => to_timescale
//====================================================================================================
uint64_t ConvertTimescale(uint64_t timestamp, uint32_t fromTimescale, uint32_t toTimescale) {
  if (fromTimescale == 0)
    return 0;

  if (fromTimescale == toTimescale) {
    return timestamp;
  }

  double ratio = static_cast<double>(toTimescale) / fromTimescale;

  return static_cast<uint64_t>(static_cast<double>(timestamp) * ratio);
}

/*
//====================================================================================================
// ConvertTimescale
// - from_timescale => to_timescale
//====================================================================================================
std::string HexStringDump(int dataSize, const uint8_t *data) {
  std::stringstream dump_stream;

  dump_stream << "\n ---------- dump(" << dataSize << "Byte) ---------- \n";

  for (int index = 0; index < dataSize; ++index) {
    if (index % 16 == 0) dump_stream << "\n";

    dump_stream << "0x" << std::setfill('0') << std::setw(2) << std::hex
                << data[index] << ' ';
  }

  dump_stream << "\n";

  return dump_stream.str();
}
*/