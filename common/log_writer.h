//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include <stdio.h>
#include <list>
#include <time.h>
#include <mutex>

#ifdef WIN32
	#include <windows.h>
	#include <direct.h>

	#pragma warning(disable: 4200)

	enum class WindowsType 
	{
		Unknown	= 0,// Unknown windows (XP, Vista, 7, etc)
		Server2003,	// Windows server 2003
		Server2008	// Windows server 2008
	};

#else
	#include <time.h>
	#include <sys/types.h>
	typedef char		TCHAR;
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
#endif

#ifndef	MAX_PATH
	#define MAX_PATH    260
#endif	 

class LogWriter
{
public:
    LogWriter(TCHAR * ptcIdent);
    ~LogWriter(void);

    bool Open(TCHAR *ptcFileName);
    void LogWrite(TCHAR *ptcFormat, ...);

    void SetDailyBackupTime(int iHour);

private:
	void WriteLogV(TCHAR *ptcFormat, va_list vl);

#ifdef WIN32
	inline WindowsType GetWindowsType();
	inline int GetWriteBufferSize(const TCHAR tcDrive, const WindowsType eType);
#endif

    FILE *		_log_file;
    TCHAR *		_file_name;

	time_t		_current_time;
	time_t		_last_write_file;

    int			_last_backup_day;
    int			_dayily_backup_hour;

#ifdef WIN32
	WindowsType _window_type;
	TCHAR		_current_drive;
	int			_write_buffer_size;
	char		*_io_buffer;
    SYSTEMTIME	_tm;    
#else	 
	struct tm	_tm;
    
#endif	 

	std::mutex _log_lock;

};


