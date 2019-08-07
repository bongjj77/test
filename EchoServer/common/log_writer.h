//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================
#pragma once

#include <stdio.h>
#include <list>
#include <time.h>

#if		defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <direct.h>

#pragma warning(disable: 4200)

typedef enum _tagWINDOWS_TYPE
{
	WINDOWS_TYPE_UNKNOWN	= 0,							// Unknown windows (XP, Vista, 7, etc)
	WINDOWS_TYPE_SERVER2003,								// Windows server 2003
	WINDOWS_TYPE_SERVER2008									// Windows server 2008
} WINDOWS_TYPE;
#else	// defined(_WIN32) || defined(_WIN64)
#include <time.h>
#include <sys/types.h>
#include <pthread.h>

typedef char    TCHAR, *LPTSTR;

#define false       0
#define true        1
#define _tcsdup     strdup
#define _stprintf   sprintf
#define _tcscpy     strcpy
#define _tcscat     strcat
#define _tfopen     fopen
#define _getpid     getpid
#define _ftprintf   fprintf
#define _vftprintf  vfprintf
#define _sntprintf	_snprintf
#define _vsntprintf	_vsnprintf
#define _trename    rename
#define _T(x)       x
#endif	// defined(_WIN32) || defined(_WIN64)

#ifndef	MAX_PATH
#define MAX_PATH    256
#endif	// MAX_PATH

class LogWriter
{
public:
    LogWriter(TCHAR *m_ptcIdent);
    ~LogWriter(void);

    bool Open(TCHAR *ptcFileName);
    void LogWrite(TCHAR *ptcFormat, ...);

    void SetDailyBackupTime(int iHour);

private:
	void WriteLogV(TCHAR *ptcFormat, va_list vl);

#if		defined(_WIN32) || defined(_WIN64)
	inline WINDOWS_TYPE GetWindowsType();
	inline int GetWriteBufferSize(const TCHAR tcDrive, const WINDOWS_TYPE eType);
#endif	// defined(_WIN32) || defined(_WIN64)

    FILE *m_hLogFile;
    TCHAR *m_ptcIdent;
    TCHAR *m_ptcFileName;

	time_t m_curTime;
	time_t m_iLastWritefile;

    int m_iLastBackupDay;
    int m_iDailyBackupHour;

#if		defined(_WIN32) || defined(_WIN64)
	WINDOWS_TYPE m_eWindowType;
	TCHAR m_tcCurDrive;
	int m_cbWriteBufferSize;
	char *m_pIoBuffer;

    SYSTEMTIME m_tm;
    CRITICAL_SECTION m_csLock;
#else	// defined(_WIN32) || defined(_WIN64)
	struct tm m_tm;
    pthread_mutex_t m_csLock;
#endif	// defined(_WIN32) || defined(_WIN64)
};


