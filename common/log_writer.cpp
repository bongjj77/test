//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "log_writer.h"
#include <stdarg.h>
#include <string.h>

//====================================================================================================
// Constructor
//====================================================================================================
LogWriter::LogWriter(char *ident)
{
    _log_file			= nullptr;
    _file_name			= nullptr;

    _last_backup_day	= -1;
    _dayily_backup_hour	= -1;

	_last_write_file	= 0;
	_current_time		= 0;
}

//====================================================================================================
// Destructor
//====================================================================================================
LogWriter::~LogWriter(void)
{
	if (_log_file != nullptr)
        fclose(_log_file);

    if (_file_name)
        free(_file_name);
}

//====================================================================================================
// Open
//====================================================================================================
bool LogWriter::Open(char *file_name)
{
    if (file_name)
    {
        if (_file_name != nullptr)
        {
            free(_file_name);
            _file_name = nullptr;
        }

        _file_name = strdup(file_name);
    }

    if (_file_name == nullptr)
        return false;

    if (_log_file != nullptr)
	{
		fclose(_log_file);
        _log_file = nullptr;
    }

    _log_file = fopen(_file_name, "ab");

    return (_log_file != nullptr ? true : false);
}

//====================================================================================================
// LogWrite
//====================================================================================================
void LogWriter::LogWrite(LogType type, const char *format, ...)
{
    va_list vl;

    va_start(vl, format);
    WriteLogV(type, format, vl);
    va_end(vl);

    return;
}

//====================================================================================================
// WriteLogV
//====================================================================================================
void LogWriter::WriteLogV(LogType type, const char *format, va_list vl)
{
	struct tm* tm;
	time_t timer;
	timer = time(nullptr);
	tm = localtime(&timer);

	char log[MAX_LOG_TEXT_SIZE] = { 0, };
	char log_line[MAX_LOG_TEXT_SIZE + 100] = { 0, };

	
	vsprintf(log, format, vl);

	sprintf(log_line, "\n%02d-%02d %02d:%02d:%02d %s%s",
		tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, g_log_type_string[(int)type], log);

	printf(log_line);
	
	std::lock_guard<std::mutex> log_lock(_log_lock);


    // Backup
    if (_dayily_backup_hour != -1)
    {
        if (_last_backup_day != tm->tm_mday && _dayily_backup_hour <= tm->tm_hour)
        {
            fclose(_log_file);
            _log_file = nullptr;

            char ptcNewFile[MAX_PATH + 1];
            char ptcBuf[64];

			sprintf(ptcBuf, "_%d%02d%02d_logbackup", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);

			strcpy(ptcNewFile, _file_name);
			strcat(ptcNewFile, ptcBuf);

            if (rename(_file_name, ptcNewFile) == 0)
            {
	              _last_backup_day  = tm->tm_mday;
            }
            Open(nullptr);
        }
    }

	if (fprintf(_log_file, log_line) < 0)
	{
		Open(nullptr);
		return;
	}

	_last_write_file = _current_time;
	fflush(_log_file);

    return;
}

//====================================================================================================
// SetDailyBackupTime
//====================================================================================================
void LogWriter::SetDailyBackupTime(int hour)
{
    if (hour < 0 || hour > 23)
    {
        _last_backup_day = -1;
        hour = -1;
    }
    else
    {
		struct tm* tm;
		time_t timer;
		timer = time(nullptr);
		tm = localtime(&timer);

        _last_backup_day = tm->tm_mday;
    }

    _dayily_backup_hour = hour;

    if (_log_file)
		LogWrite(LogType::Info, "Set Daily Backup : %d", _dayily_backup_hour);

    return;
}