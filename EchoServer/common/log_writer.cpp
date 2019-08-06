#include "log_writer.h"
#include <stdarg.h>
#if defined(_WIN32) || defined(_WIN64)
#include <process.h>
#include <tchar.h>
#else
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

#if		(defined(_WIN32) || defined(_WIN64))
#	define	LOCK_LOG()						::EnterCriticalSection(&m_csLock)
#	define	UNLOCK_LOG()					::LeaveCriticalSection(&m_csLock)
#else	// (defined(_WIN32) || defined(_WIN64))
#	define	LOCK_LOG()						pthread_mutex_lock(&m_csLock)
#	define	UNLOCK_LOG()					pthread_mutex_unlock(&m_csLock)
#endif	// (defined(_WIN32) || defined(_WIN64))
 
LogWriter::LogWriter(TCHAR *ptcIdent)
{
    m_ptcIdent			= (ptcIdent != nullptr ? _tcsdup(ptcIdent) : nullptr);
    m_hLogFile			= nullptr;
    m_ptcFileName		= nullptr;

    m_iLastBackupDay	= -1;
    m_iDailyBackupHour	= -1;

	m_iLastWritefile	= 0;		// 최소 파일 기록 시간, 초단위
	m_curTime			= 0;
#if		defined(_WIN32) || defined(_WIN64)

	m_eWindowType		= GetWindowsType();
	m_tcCurDrive		= _T('D');
	m_cbWriteBufferSize	= GetWriteBufferSize(m_tcCurDrive, m_eWindowType);
	m_pIoBuffer			= (0 < m_cbWriteBufferSize ? (char*)malloc(m_cbWriteBufferSize) : nullptr);
			
	::InitializeCriticalSection(&m_csLock);

#if		_DEC_WRITE_TIME_CHECK
	// for test
	TCHAR szAlert[1024];
	_stprintf(szAlert, _T("$$$$ Windows type is [%s]"), 
		WINDOWS_TYPE_SERVER2008 == m_eWindowType ? _T("Windows server 2008") : 
		WINDOWS_TYPE_SERVER2003 == m_eWindowType ? _T("Windows server 2003") : 
		_T("Unknown windows"));
	OutputDebugString(szAlert);
#endif	// _DEC_WRITE_TIME_CHECK
	
#else	// defined(_WIN32) || defined(_WIN64)
    pthread_mutex_init(&m_csLock, nullptr);
#endif	// defined(_WIN32) || defined(_WIN64)
}


LogWriter::~LogWriter(void)
{
	if (m_hLogFile != nullptr)
        fclose(m_hLogFile);

#if		defined(_WIN32) || defined(_WIN64)
	if (m_pIoBuffer)
		free(m_pIoBuffer);
#endif	// defined(_WIN32) || defined(_WIN64)

    if (m_ptcIdent)
        free(m_ptcIdent);

    if (m_ptcFileName)
        free(m_ptcFileName);

#if		defined(_WIN32) || defined(_WIN64)
    DeleteCriticalSection(&m_csLock);
#else	// defined(_WIN32) || defined(_WIN64)
    pthread_mutex_destroy(&m_csLock);
#endif	// defined(_WIN32) || defined(_WIN64)
}


bool LogWriter::Open(TCHAR *ptcFileName)
{
    if (ptcFileName)
    {
        if (m_ptcFileName != nullptr)
        {
            free(m_ptcFileName);
            m_ptcFileName = nullptr;
        }

        m_ptcFileName = _tcsdup(ptcFileName);
    }

    if (m_ptcFileName == nullptr)
        return false;

    if (m_hLogFile != nullptr)
	{
		fclose(m_hLogFile);
        m_hLogFile = nullptr;
    }

#if		defined(_WIN32) || defined(_WIN64)
	if(m_tcCurDrive != m_ptcFileName[0])
	{
		if (m_pIoBuffer)
			free(m_pIoBuffer);

		m_cbWriteBufferSize	= GetWriteBufferSize(m_ptcFileName[0], m_eWindowType);
		m_pIoBuffer			= (0 < m_cbWriteBufferSize ? (char*)malloc(m_cbWriteBufferSize) : nullptr);
		m_tcCurDrive		= m_ptcFileName[0];
	}
#endif	// defined(_WIN32) || defined(_WIN64)

    m_hLogFile = _tfopen(m_ptcFileName, _T("ab"));
	if (m_hLogFile)
	{
#if		((defined(_WIN32) || defined(_WIN64)))
		if (0 < m_cbWriteBufferSize && m_pIoBuffer)
			setvbuf(m_hLogFile, m_pIoBuffer, _IOFBF, m_cbWriteBufferSize);

#ifdef	_UNICODE
		fseek(m_hLogFile, 0, SEEK_END);

		if (ftell(m_hLogFile) == 0)
			fprintf(m_hLogFile, "%c%c", 0xff, 0xfe);
#endif	// _UNICODE
#endif	// ((defined(_WIN32) || defined(_WIN64)))
	}

    return (m_hLogFile != nullptr ? true : false);
}


void LogWriter::LogWrite(TCHAR *ptcFormat, ...)
{
    va_list vl;

    va_start(vl, ptcFormat);
    WriteLogV(ptcFormat, vl);
    va_end(vl);

    return;
}

void LogWriter::WriteLogV(TCHAR *ptcFormat, va_list vl)
{
	LOCK_LOG();

	m_curTime = time(nullptr);
	// Get current time
#if		defined(_WIN32) || defined(_WIN64)
    ::GetLocalTime(&m_tm);
#else	// defined(_WIN32) || defined(_WIN64)
    localtime_r(&m_curTime, &m_tm);
#endif	// defined(_WIN32) || defined(_WIN64)

    // Backup
    if (m_iDailyBackupHour != -1)
    {
#if		defined(_WIN32) || defined(_WIN64)
        if (m_iLastBackupDay != m_tm.wDay && m_iDailyBackupHour <= m_tm.wHour)
#else	// defined(_WIN32) || defined(_WIN64)
        if (m_iLastBackupDay != m_tm.tm_mday && m_iDailyBackupHour <= m_tm.tm_hour)
#endif	// defined(_WIN32) || defined(_WIN64)
        {
            fclose(m_hLogFile);
            m_hLogFile = nullptr;

            TCHAR ptcNewFile[MAX_PATH + 1];
            TCHAR ptcBuf[64];

#if		defined(_WIN32) || defined(_WIN64)
            _stprintf(ptcBuf, _T("_%d%02d%02d_logbackup"), m_tm.wYear, m_tm.wMonth, m_tm.wDay);
#else	// defined(_WIN32) || defined(_WIN64)
            _stprintf(ptcBuf, "_%d%02d%02d_logbackup", m_tm.tm_year + 1900, m_tm.tm_mon + 1, m_tm.tm_mday);
#endif	// defined(_WIN32) || defined(_WIN64)

            _tcscpy(ptcNewFile, m_ptcFileName);
            _tcscat(ptcNewFile, ptcBuf);

            if (_trename(m_ptcFileName, ptcNewFile) == 0)
            {
#if		defined(_WIN32) || defined(_WIN64)
                m_iLastBackupDay  = m_tm.wDay;
#else	// defined(_WIN32) || defined(_WIN64)
                m_iLastBackupDay  = m_tm.tm_mday;
#endif	// defined(_WIN32) || defined(_WIN64)
            }
            Open(nullptr);
        }
    }

	if (0 > _ftprintf(m_hLogFile, _T("\n%02d-%02d %02d:%02d:%02d "), 
#if		defined(_WIN32) || defined(_WIN64)
							m_tm.wMonth, m_tm.wDay, m_tm.wHour, m_tm.wMinute, m_tm.wSecond))
#else	// defined(_WIN32) || defined(_WIN64)
							m_tm.tm_mon + 1, m_tm.tm_mday, m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec))
#endif	// defined(_WIN32) || defined(_WIN64))
        goto BAIL;

    if (0 > _vftprintf(m_hLogFile, ptcFormat, vl))
        goto BAIL;

#ifndef _DEBUG
	if (m_curTime > m_iLastWritefile + 3)		// 최소 파일 기록 시간, 10초단위
#endif // _DEBUG
	{
		m_iLastWritefile = m_curTime;
		fflush(m_hLogFile);
	}

	UNLOCK_LOG();
    return;

BAIL:	
	UNLOCK_LOG();
    Open(nullptr);
}

void LogWriter::SetDailyBackupTime(int iHour)
{
    if (iHour < 0 || iHour > 23)
    {
        m_iLastBackupDay = -1;
        iHour = -1;
    }
    else
    {
#if		defined(_WIN32) || defined(_WIN64)
        ::GetLocalTime(&m_tm);
        m_iLastBackupDay = m_tm.wDay;
#else	// defined(_WIN32) || defined(_WIN64)
        time_t curTime;

        curTime = time(nullptr);
        localtime_r(&curTime, &m_tm);
        m_iLastBackupDay = m_tm.tm_mday;
#endif	// defined(_WIN32) || defined(_WIN64)
    }

    m_iDailyBackupHour = iHour;

    if (m_hLogFile)
		LogWrite(_T("Set Daily Backup : %d"), m_iDailyBackupHour);

    return;
}

#if		defined(_WIN32) || defined(_WIN64)

////////////////////////////////////////////////////////////////////////////////////////
// Windows version check
// Ref. URL: http://msdn.microsoft.com/en-us/library/windows/desktop/ms724429(v=vs.85).aspx
WINDOWS_TYPE LogWriter::GetWindowsType()
{
	OSVERSIONINFOEX	osvi = {sizeof(osvi), 0, };

	// Get OS version information
	if (!::GetVersionEx((LPOSVERSIONINFO)&osvi))
	{
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		if (!::GetVersionEx((LPOSVERSIONINFO)&osvi))				{ return WINDOWS_TYPE_UNKNOWN; }
	}

	// Check flatform ID
	if (VER_PLATFORM_WIN32_NT <= osvi.dwPlatformId)
	{
		// Check windows version
		if (5 > osvi.dwMajorVersion)								{ return WINDOWS_TYPE_UNKNOWN; }
		else if (5 == osvi.dwMajorVersion)
		{
			if (1 >= osvi.dwMinorVersion)							{ return WINDOWS_TYPE_UNKNOWN; }
			else
			{
				if (VER_NT_SERVER == osvi.wProductType)				{ return WINDOWS_TYPE_SERVER2003; }
				else												{ return WINDOWS_TYPE_UNKNOWN; }
			}
		}
		else
		{
			// It's same I/O logic, if major version is 6 or upper.
			return WINDOWS_TYPE_SERVER2008;
		}
	}

	return WINDOWS_TYPE_UNKNOWN;
}

int LogWriter::GetWriteBufferSize(const TCHAR tcDrive, const WINDOWS_TYPE eWinType)
{
	struct	_diskfree_t df	= { 0, };
	int		mul_factor;

	// Get disk cluster, sector information
	if (!_istascii(tcDrive) || !_istalpha(tcDrive) || _getdiskfree(_totupper(tcDrive) - _T('A') + 1, &df))
	{
		df.sectors_per_cluster	= 8;
		df.bytes_per_sector		= 512;
	}

	// Get multification factor by windows type
	switch(eWinType)
	{
	case WINDOWS_TYPE_SERVER2003:	mul_factor = 10;	break;
	case WINDOWS_TYPE_SERVER2008:	mul_factor = 1024;	break;
	default:						mul_factor = 0;		break;
	}

	return df.bytes_per_sector * df.sectors_per_cluster * mul_factor;
}

#endif	// defined(_WIN32) || defined(_WIN64)