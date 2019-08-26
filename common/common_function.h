//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once

#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
	#include <windows.h>
	#include <winsock2.h>
	#pragma comment (lib, "rpcrt4.lib") // for UuidCreate
#endif

#include <time.h>
#include <string>
#include <vector> 
#include <map>
#include <queue>
#include <deque>
#include <stdio.h>
#include "log_writer.h" 

//------------------------------ log define ------------------------------ 
#define LOG_WRITE(format)			LogPrint format
#define LOG_ERROR_WRITE(format)		LogErrorPrint format
#define LOG_WARNING_WRITE(format)	LogWarningPrint format
#define LOG_INFO_WRITE(format)		LogInfoPrint format
#define LOG_DEBUG_WRITE(format)     LogPrint format

extern bool LogInit(const char * file_name, int nBackupHour); 
extern void LogPrint(const char *str, ...);
extern void LogErrorPrint(const char* str, ...);
extern void LogWarningPrint(const char* str, ...);
extern void LogInfoPrint(const char* str, ...);

//
extern void 		InitNetwork();
extern void 		ReleaseNetwork();
extern std::string	string_format(const char* format, ...);
extern void			Tokenize(const std::string & str, std::vector<std::string> & tokens, const std::string& delimiters);
extern void			Tokenize2(const char * text, std::vector<std::string>& tokens, char delimiter);
extern std::string 	GetStringIP(uint32_t ip); 
extern std::string	GetStringTime(time_t time_value); 
extern std::string 	GetStringTime2(time_t time_value, bool bDate = true);
extern std::string 	GetStringDate(time_t time_value, const char * format = nullptr); 

extern void 		ReplaceString( std::string & strText, const char * pszBefore, const char * pszAfter );
extern void			StringNCopy(char * pDest, const char * pSource, int pMaxLength); 
extern uint32_t GetAddressIP(const char * pAddress);
extern bool 		GetUUID(std::string & strUUID);
extern bool			IsEmptyString(char * pString);
extern int			GetCpuCount();
extern void			MakeDirectory(const char * pPath); 
extern int 			RoundAdjust(double nNumber);
extern bool			FileExist(const char * pFilePath);
extern void			SleepWait(int nMillisecond); 
extern uint64_t 	GetCurrentTick();
double				GetCurrentMilliseconds();
extern int			GetLocalAddress(char * pAddress);
extern std::string	GetLocalHostName();
extern std::string	RandomString(uint32_t size);
extern std::string	RandomNumberString(uint32_t size);
extern uint32_t Gcd(uint32_t n1, uint32_t n2);
extern uint64_t ConvertTimescale(uint64_t time, uint32_t from_timescale, uint32_t to_timescale);
