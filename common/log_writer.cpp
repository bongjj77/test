//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "log_writer.h"
#include <stdarg.h>
#ifdef WIN32
	#include <process.h>
	#include <tchar.h>
#else
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
#endif
 
LogWriter::LogWriter(TCHAR *ptcIdent)
{
    _log_file			= nullptr;
    _file_name		= nullptr;

    _last_backup_day	= -1;
    _dayily_backup_hour	= -1;

	_last_write_file	= 0;		// 최소 파일 기록 시간, 초단위
	_current_time		= 0;

#ifdef WIN32

	_window_type		= GetWindowsType();
	_current_drive		= _T('D');
	_write_buffer_size	= GetWriteBufferSize(_current_drive, _window_type);
	_io_buffer			= (0 < _write_buffer_size ? (char*)malloc(_write_buffer_size) : nullptr);
	

	#if _DEC_WRITE_TIME_CHECK
		// for test
		TCHAR szAlert[1024];
		_stprintf(szAlert, _T("$$$$ Windows type is [%s]"), 
			Server2008 == _window_type ? _T("Windows server 2008") : 
			Server2003 == _window_type ? _T("Windows server 2003") : 
			_T("Unknown windows"));
		OutputDebugString(szAlert);
	#endif	// _DEC_WRITE_TIME_CHECK

#endif
}


LogWriter::~LogWriter(void)
{
	if (_log_file != nullptr)
        fclose(_log_file);

#ifdef WIN32
	if (_io_buffer)
		free(_io_buffer);
#endif

    if (_file_name)
        free(_file_name);

}


bool LogWriter::Open(TCHAR *ptcFileName)
{
    if (ptcFileName)
    {
        if (_file_name != nullptr)
        {
            free(_file_name);
            _file_name = nullptr;
        }

        _file_name = _tcsdup(ptcFileName);
    }

    if (_file_name == nullptr)
        return false;

    if (_log_file != nullptr)
	{
		fclose(_log_file);
        _log_file = nullptr;
    }

#ifdef WIN32
	if(_current_drive != _file_name[0])
	{
		if (_io_buffer)
			free(_io_buffer);

		_write_buffer_size	= GetWriteBufferSize(_file_name[0], _window_type);
		_io_buffer			= (0 < _write_buffer_size ? (char*)malloc(_write_buffer_size) : nullptr);
		_current_drive		= _file_name[0];
	}
#endif 

    _log_file = _tfopen(_file_name, _T("ab"));
	if (_log_file)
	{
#ifdef WIN32
		if (0 < _write_buffer_size && _io_buffer)
			setvbuf(_log_file, _io_buffer, _IOFBF, _write_buffer_size);

#ifdef	_UNICODE
		fseek(_log_file, 0, SEEK_END);

		if (ftell(_log_file) == 0)
			fprintf(_log_file, "%c%c", 0xff, 0xfe);
#endif	// _UNICODE
#endif	 
	}

    return (_log_file != nullptr ? true : false);
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
	std::lock_guard<std::mutex> log_lock(_log_lock);

	_current_time = time(nullptr);
	// Get current time
#ifdef WIN32
    ::GetLocalTime(&_tm);
#else	 
    localtime_r(&_current_time, &_tm);
#endif	 

    // Backup
    if (_dayily_backup_hour != -1)
    {
#ifdef WIN32
        if (_last_backup_day != _tm.wDay && _dayily_backup_hour <= _tm.wHour)
#else	 
        if (_last_backup_day != _tm.tm_mday && _dayily_backup_hour <= _tm.tm_hour)
#endif
        {
            fclose(_log_file);
            _log_file = nullptr;

            TCHAR ptcNewFile[MAX_PATH + 1];
            TCHAR ptcBuf[64];

#ifdef WIN32
            _stprintf(ptcBuf, _T("_%d%02d%02d_logbackup"), _tm.wYear, _tm.wMonth, _tm.wDay);
#else 
            _stprintf(ptcBuf, "_%d%02d%02d_logbackup", _tm.tm_year + 1900, _tm.tm_mon + 1, _tm.tm_mday);
#endif 

            _tcscpy(ptcNewFile, _file_name);
            _tcscat(ptcNewFile, ptcBuf);

            if (_trename(_file_name, ptcNewFile) == 0)
            {
#ifdef WIN32
                _last_backup_day  = _tm.wDay;
#else	 
                _last_backup_day  = _tm.tm_mday;
#endif 
            }
            Open(nullptr);
        }
    }

	if (0 > _ftprintf(_log_file, _T("\n%02d-%02d %02d:%02d:%02d "), 
#ifdef WIN32
		_tm.wMonth, _tm.wDay, _tm.wHour, _tm.wMinute, _tm.wSecond))
#else	 
		_tm.tm_mon + 1, _tm.tm_mday, _tm.tm_hour, _tm.tm_min, _tm.tm_sec))
#endif 
        goto BAIL;

    if (0 > _vftprintf(_log_file, ptcFormat, vl))
        goto BAIL;

#ifndef _DEBUG
	if (_current_time > _last_write_file + 3)		// 최소 파일 기록 시간, 10초단위
#endif // _DEBUG
	{
		_last_write_file = _current_time;
		fflush(_log_file);
	}

    return;

BAIL:	

    Open(nullptr);
}

void LogWriter::SetDailyBackupTime(int iHour)
{
    if (iHour < 0 || iHour > 23)
    {
        _last_backup_day = -1;
        iHour = -1;
    }
    else
    {
#ifdef WIN32
        ::GetLocalTime(&_tm);
        _last_backup_day = _tm.wDay;
#else 
        time_t curTime;

        curTime = time(nullptr);
        localtime_r(&curTime, &_tm);
        _last_backup_day = _tm.tm_mday;
#endif
    }

    _dayily_backup_hour = iHour;

    if (_log_file)
		LogWrite(_T("Set Daily Backup : %d"), _dayily_backup_hour);

    return;
}

#ifdef WIN32

////////////////////////////////////////////////////////////////////////////////////////
// Windows version check
// Ref. URL: http://msdn.microsoft.com/en-us/library/windows/desktop/ms724429(v=vs.85).aspx
WindowsType LogWriter::GetWindowsType()
{
	OSVERSIONINFOEX	osvi = {sizeof(osvi), 0, };

	// Get OS version information
	if (!::GetVersionEx((LPOSVERSIONINFO)&osvi))
	{
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		if (!::GetVersionEx((LPOSVERSIONINFO)&osvi))				{ return WindowsType::Unknown; }
	}

	// Check flatform ID
	if (VER_PLATFORM_WIN32_NT <= osvi.dwPlatformId)
	{
		// Check windows version
		if (5 > osvi.dwMajorVersion)								{ return WindowsType::Unknown; }
		else if (5 == osvi.dwMajorVersion)
		{
			if (1 >= osvi.dwMinorVersion)							{ return WindowsType::Unknown; }
			else
			{
				if (VER_NT_SERVER == osvi.wProductType)				{ return WindowsType::Server2003; }
				else												{ return WindowsType::Unknown; }
			}
		}
		else
		{
			// It's same I/O logic, if major version is 6 or upper.
			return WindowsType::Server2008;
		}
	}

	return WindowsType::Unknown;
}

int LogWriter::GetWriteBufferSize(const TCHAR tcDrive, const WindowsType eWinType)
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
	case WindowsType::Server2003:	mul_factor = 10;	break;
	case WindowsType::Server2008:	mul_factor = 1024;	break;
	default:						mul_factor = 0;		break;
	}

	return df.bytes_per_sector * df.sectors_per_cluster * mul_factor;
}

#endif