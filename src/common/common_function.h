#pragma once
#include <deque>
#include <map>
#include <memory>
#include <queue>
#include <stdio.h>
#include <string>
#include <time.h>
#include <vector>

extern std::string GetStringTime(time_t timeValue, bool isDate = true);

extern uint32_t GetAddressIp(const char *address);

extern uint64_t GetCurrentTick();

extern uint64_t GetCurrentMs();

extern std::string GetLocalHostName();

extern uint64_t ConvertTimescale(uint64_t timestamp, uint32_t from_timescale, uint32_t to_timescale);

extern std::string HostToIp(const std::string &host);

// extern std::string HexStringDump(int dataSize, const uint8_t* data);
