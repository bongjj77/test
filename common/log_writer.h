//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include <stdio.h>
#include <list>
#include <time.h>
#include <mutex>

#ifndef	MAX_PATH
	#define MAX_PATH    260
#endif	 

#define MAX_LOG_TEXT_SIZE	(4096)

enum class LogType
{
	Custom = 0,
	Info,
	Warning,
	Error,

	MAX,
};

static char g_log_type_string[(int)LogType::MAX][20] =
{
	"",			// Custom = 0,
	"INFO : ",	// Info,
	"WARNING : ",// Warning,
	"ERROR : ",	// Error,
};
//====================================================================================================
// LogWriter
//====================================================================================================
class LogWriter
{
public:
    LogWriter(char * ident);
    ~LogWriter(void);

    bool Open(char *file_name);
    void LogWrite(LogType type, const char *format, ...);
	void WriteLogV(LogType type, const char* format, va_list vl);
    void SetDailyBackupTime(int hour);
	void SetLogLevel(LogType level_type) { _level_type = level_type; }
	LogType GetLogLevel(LogType level_type) { return _level_type; }

private:

    FILE * _log_file;
    char * _file_name;

	time_t _current_time;
	time_t _last_write_file;

    int _last_backup_day;
    int _dayily_backup_hour;
	std::mutex	_log_lock;
	LogType _level_type = LogType::Custom;
};


