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
#define LOG_INIT(file_name, nBackupHour) LogInit(file_name, nBackupHour)
#define LOG_WRITE(pFormat) LogPrint pFormat

#define MAX_LOG_TEXT_SIZE	(4096)
extern bool LogInit(const char * file_name, int nBackupHour); 
extern void LogPrint(const char *str, ...);

//
extern void 		InitNetwork();
extern void 		ReleaseNetwork();
extern void			Tokenize(const std::string & str, std::vector<std::string> & tokens, const std::string& delimiters);
extern void			Tokenize2(const char * pText, std::vector<std::string>& tokens, char delimiter);
extern std::string 	GetStringIP(uint32_t ip); 
extern void			GetStringTime(std::string & time_string, time_t time_value); 
extern std::string 	GetStringTime2(time_t time_value, bool bDate = true);
extern std::string 	GetStringDate(time_t time_value, const char * pFormat = nullptr); 

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
extern uint32_t 	get_tick_count();
extern uint64_t 	get_tick_count64();
extern int			GetLocalAddress(char * pAddress);
extern bool			GetLocalHostName(std::string & host_name);
extern std::string	RandomString(uint32_t size);
extern std::string	RandomNumberString(uint32_t size);