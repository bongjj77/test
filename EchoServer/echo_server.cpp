#include "stdafx.h"

#ifndef _WIN32
#include <fcntl.h>
#include <iostream>
#include <string>
#include <log_writer.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#endif
#include <signal.h>
#include "main_object.h"

#define _VERSION_ 		"1.0.000"
#define _PROGREAM_NAME_ "echo_server"

bool gRun = true;

//====================================================================================================
// 시그널 핸들러 
//====================================================================================================
void SignalHandler(int sig)
{
	gRun = false;
	
	// ignore all these signals now and let the connection close
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
}

//====================================================================================================
// 기본 설정 정보 출력 
//====================================================================================================
void SettingPrint( )
{

	LOG_WRITE(("=================================================="));
	LOG_WRITE(("[ %s Config]", 		_PROGREAM_NAME_));
	LOG_WRITE(("	- %s : %s", 	CONFIG_VERSION, 					GET_CONFIG_VALUE(CONFIG_VERSION)));
	LOG_WRITE(("	- %s : %s",		CONFIG_THREAD_POOL_COUNT,			GET_CONFIG_VALUE(CONFIG_THREAD_POOL_COUNT)));
	LOG_WRITE(("	- %s : %s", 	CONFIG_DEBUG_MODE, 					GET_CONFIG_VALUE(CONFIG_DEBUG_MODE)));
	LOG_WRITE(("	- %s : %s", 	CONFIG_SYS_LOG_BACKUP_HOUR, 		GET_CONFIG_VALUE(CONFIG_SYS_LOG_BACKUP_HOUR)));
	LOG_WRITE(("	- %s : %s", 	CONFIG_HOST_NAME,					GET_CONFIG_VALUE(CONFIG_HOST_NAME)));
	LOG_WRITE(("=================================================="));
}

//====================================================================================================
// 설정 파일 로드 
//====================================================================================================
bool LoadConfigFile(char * szProgramPath)
{
	// Confit 파일 경로 설정 
	char szConfigFile[300] = {0, };
	
	
#ifndef _WIN32
	char szDelimiter = '/';
#else
	char szDelimiter = '\\';
#endif

	const char* pszAppName = strrchr(szProgramPath, szDelimiter);

	if (nullptr != pszAppName)
	{
		int folderlenth = strlen(szProgramPath) - strlen(pszAppName) + 1;
		strncpy(szConfigFile, szProgramPath, folderlenth);
	}
	strcat(szConfigFile, DEFAULT_CONFIG_FILE);
	

	// Config 파일 로드 
	if (CONFIG_PARSER.LoadFile(szConfigFile) == false) 
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
	// 설정 파일 로드 
	if (LoadConfigFile(argv[0]) == false) 
	{
		fprintf(stderr, "Config file not found \r\n");
		return -1; 
	}
	
	signal(SIGINT, SignalHandler);
	signal(SIGTERM, SignalHandler);
	
	//리눅스 데몬 처리 및 로그 초기화 
#ifndef _WIN32
    signal(SIGCLD,SIG_IGN);
#endif
 
	//로그 파일 초기화 
	if(LOG_INIT(GET_CONFIG_VALUE(CONFIG_LOG_FILE_PATH), atoi(GET_CONFIG_VALUE(CONFIG_SYS_LOG_BACKUP_HOUR))) == false)
	{	
		fprintf(stderr,"%s Log System Error \n", _PROGREAM_NAME_ );  
		return -1; 
	}
	 
	//네트워크 초기화 
    InitNetwork();

	//기본 설정 정보 출력 
	SettingPrint( );

	//생성 파라메터 설정 
	CreateParam param;
	strcpy(param.version, 				GET_CONFIG_VALUE(CONFIG_VERSION));
	param.thread_pool_count				= atoi(GET_CONFIG_VALUE(CONFIG_THREAD_POOL_COUNT));
	param.debug_mode					= atoi(GET_CONFIG_VALUE(CONFIG_DEBUG_MODE)) == 1 ? true : false;
	strcpy(param.host_name, 			GET_CONFIG_VALUE(CONFIG_HOST_NAME)); 
	param.network_bandwidth				= atoi(GET_CONFIG_VALUE(CONFIG_NETWORK_BAND_WIDTH)); 
	param.local_ip						= inet_addr(GET_CONFIG_VALUE(CONFIG_LOCAL_IP));
		
	//MainObject 생성 
	MainObject main_object;

   	if(main_object.Create(&param) == false)
	{
		LOG_WRITE(("[ %s ] MainObject Create Error\r\n", _PROGREAM_NAME_));
		return -1;
	}
 
	std::string time_string; 

	GetStringTime(time_string, 0);
	LOG_WRITE(("===== [ %s Start(%s)] =====", _PROGREAM_NAME_, time_string.c_str()));

	//프로세스 대기 루프 및 종료 처리  
	while(gRun == true)
	{
#ifndef _WIN32
		Sleep(100);
#else
        if(getchar() == 'x') 
		{
			gRun = false;
        }
#endif
	}

	GetStringTime(time_string, 0);
	LOG_WRITE(("===== [ %s End(%s) ] =====", _PROGREAM_NAME_, time_string.c_str()));
	
	CONFIG_PARSER.UnloadFile();

    //네트워크 종료 
    ReleaseNetwork();

	return 0;
}



