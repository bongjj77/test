//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#ifdef WIN32

	#ifndef VC_EXTRALEAN
		#define VC_EXTRALEAN		// Windows 헤더에서 거의 사용되지 않는 내용을 제외시킵니다.
	#endif

	#ifdef _DEBUG 
		#define _CRTDBG_MAP_ALLOC
		#define _CRTDBG_MAP_ALLOC_NEW
		#include <crtdbg.h>
		#define CHECK_MEMORY_LEAK		_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
		#define CHECK_BREAK_POINT(a)	_CrtSetBreakAlloc( a );
	#else
		#define CHECK_MEMORY_LEAK
		#define CHECK_BREAK_POINT(a)
	#endif  

	#define _STDINT_H
	#pragma warning(disable:4200)	// zero-sized array warning
	#pragma warning(disable:4996)	// ( vsprintf, strcpy, fopen ) function unsafe
	#pragma warning(disable:4715)	// return value not exist
	#pragma warning(disable:4244)
	#pragma warning(disable:4717)
	#pragma warning(disable:4018)	// signed/unsigned mismatch
	#pragma warning(disable:4005)	// macro redefinition
	#pragma warning(disable:94)		// the size of an array must be greater than zero

#else
	#define CHECK_MEMORY_LEAK7
	#define CHECK_BREAK_POINT(a)
#endif

#include "config_define.h"
#include "common/common_header.h" 
#include <signal.h>
#include "main_object.h"

#define _VERSION_ 		"1.0.000"
#define _PROGREAM_NAME_ "media_streaming_server"

bool g_run = true;

//====================================================================================================
// 시그널 핸들러 
//====================================================================================================
void SignalHandler(int sig)
{
	g_run = false;
	
	// ignore all these signals now and let the connection close
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
}

//====================================================================================================
// 기본 설정 정보 출력 
//====================================================================================================
void SettingPrint( )
{

	LOG_WRITE(("===================================================================================================="));
	LOG_WRITE(("[ %s Config]", 		_PROGREAM_NAME_));
	LOG_WRITE(("    - %s : %s", CONFIG_VERSION, 			GET_CONFIG_VALUE(CONFIG_VERSION)));
	LOG_WRITE(("    - %s : %s",	CONFIG_THREAD_POOL_COUNT,	GET_CONFIG_VALUE(CONFIG_THREAD_POOL_COUNT)));
	LOG_WRITE(("    - %s : %s", CONFIG_DEBUG_MODE, 			GET_CONFIG_VALUE(CONFIG_DEBUG_MODE)));
	LOG_WRITE(("    - %s : %s", CONFIG_SYS_LOG_BACKUP_HOUR, GET_CONFIG_VALUE(CONFIG_SYS_LOG_BACKUP_HOUR)));
	LOG_WRITE(("    - %s : %s", CONFIG_HOST_NAME,			GET_CONFIG_VALUE(CONFIG_HOST_NAME)));
	LOG_WRITE(("    - %s : %s", CONFIG_RTMP_LISTEN_PORT,	GET_CONFIG_VALUE(CONFIG_RTMP_LISTEN_PORT)));
	LOG_WRITE(("    - %s : %s", CONFIG_HTTP_LISTEN_PORT,	GET_CONFIG_VALUE(CONFIG_HTTP_LISTEN_PORT)));
	LOG_WRITE(("===================================================================================================="));
}

//====================================================================================================
// 설정 파일 로드 
//====================================================================================================
bool LoadConfigFile(char * program_path)
{
	// Confit 파일 경로 설정 
	char config_file_path[MAX_PATH] = {0, };
		
#ifndef _WIN32
	char delimiter = '/';
#else
	char delimiter = '\\';
#endif

	const char* application_name = strrchr(program_path, delimiter);

	if (nullptr != application_name)
	{
		int folderlenth = strlen(program_path) - strlen(application_name) + 1;
		strncpy(config_file_path, program_path, folderlenth);
	}

	strncat(config_file_path, DEFAULT_CONFIG_FILE, MAX_PATH);
	
	// Config 파일 로드 
	if (ConfigParser::GetInstance().LoadFile(config_file_path) == false)
	{
		return false;
	}

	return true; 
}

//====================================================================================================
// Main
//====================================================================================================
int main(int argc, char* argv[])
{	
	// config file load
	if (LoadConfigFile(argv[0]) == false) 
	{
		fprintf(stderr, "Config file not found \r\n");
		//return -1; 
	}
	
	signal(SIGINT, SignalHandler);
	signal(SIGTERM, SignalHandler);
 
#ifndef _WIN32
    signal(SIGCLD,SIG_IGN);
#endif
 
	// Log file init
	if(LogInit(GET_CONFIG_VALUE(CONFIG_LOG_FILE_PATH), atoi(GET_CONFIG_VALUE(CONFIG_SYS_LOG_BACKUP_HOUR))) == false)
	{	
		fprintf(stderr,"%s Log System Error \n", _PROGREAM_NAME_ );  
		return -1; 
	}
	 
	// network init
    InitNetwork();

	// setting info out
	SettingPrint( );

	// setting crate param
	auto create_param = std::make_unique<CreateParam>();
	std::string host_name = GetLocalHostName();

	create_param->version			= GET_CONFIG_VALUE(CONFIG_VERSION);
	create_param->thread_pool_count	= atoi(GET_CONFIG_VALUE(CONFIG_THREAD_POOL_COUNT));
	create_param->debug_mode		= atoi(GET_CONFIG_VALUE(CONFIG_DEBUG_MODE)) == 1 ? true : false;
	create_param->host_name			= GET_CONFIG_VALUE(CONFIG_HOST_NAME);
	create_param->network_bandwidth	= atoi(GET_CONFIG_VALUE(CONFIG_NETWORK_BAND_WIDTH));
	create_param->local_ip			= inet_addr(GET_CONFIG_VALUE(CONFIG_LOCAL_IP));
	create_param->rtmp_listen_port	= atoi(GET_CONFIG_VALUE(CONFIG_RTMP_LISTEN_PORT));
	create_param->http_listen_port	= atoi(GET_CONFIG_VALUE(CONFIG_HTTP_LISTEN_PORT));
	create_param->start_time		= time(nullptr);
	create_param->real_host_name	= host_name.empty() == false ? host_name : create_param->host_name;

	LOG_INFO_WRITE(("Host Name - %s(%s) ", create_param->host_name.c_str(), create_param->real_host_name.c_str()));
 
	// Main object create
	auto main_object = std::make_unique<MainObject>();

   	if(main_object->Create(std::move(create_param)) == false)
	{
		LOG_WRITE(("[ %s ] MainObject Create Error", _PROGREAM_NAME_));
		return -1;
	}
   
	LOG_WRITE(("========== [ %s Start(%s)] ==========", _PROGREAM_NAME_, GetStringTime(0).c_str()));

	// process main loop
	while(g_run == true)
	{
#ifndef WIN32
		SleepWait(100);
#else
        if(getchar() == 'x') 
		{
			g_run = false;
        }
#endif
	}

	LOG_WRITE(("========== [ %s End(%s) ] ==========", _PROGREAM_NAME_, GetStringTime(0).c_str()));
	
	ConfigParser::GetInstance().UnloadFile();

    // network release
    ReleaseNetwork();

	return 0;
}