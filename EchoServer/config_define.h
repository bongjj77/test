//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================
#pragma once
 
#define DEFAULT_CONFIG_FILE									"echo_server.conf"
 
//------------------------------ 설정 이름 ------------------------------
#define CONFIG_VERSION							"version"
#define CONFIG_THREAD_POOL_COUNT				"thread_pool_count"
#define CONFIG_DEBUG_MODE						"debug_mode"
#define	CONFIG_SYS_LOG_BACKUP_HOUR				"sys_log_backup_hour"
#define CONFIG_LOG_FILE_PATH					"log_file_path" 
#define CONFIG_KEEPALIVE_TIME					"keepalive_time" 
#define CONFIG_HTTP_CLIENT_KEEPALIVE_TIME		"http_client_keepalive_time" 	
#define CONFIG_HOST_NAME						"host_name"
#define CONFIG_NETWORK_BAND_WIDTH				"network_band_width"
#define CONFIG_LOCAL_IP							"local_ip"
#define CONFIG_FILE_SAVE_PATH					"file_save_path" 
#define CONFIG_ETHERNET							"ethernet"

//------------------------------ 기본  값 ------------------------------
#define CONFIG_THREAD_POOL_COUNT_VALUE				"0" // 0 - cpu count
#define CONFIG_FILE_SAVE_PATH_VALUE					"./" 
#define CONFIG_LOG_FILE_PATH_VALUE					"./log/echo_server.log"
#define CONFIG_VERSION_VALUE						"V_2019"
#define CONFIG_DEBUG_MODE_VALUE						"0"
#define	CONFIG_SYS_LOG_BACKUP_HOUR_VALUE			"4"
#define CONFIG_KEEPALIVE_TIME_VALUE					"300" 
#define CONFIG_HTTP_CLIENT_KEEPALIVE_TIME_VALUE		"100"//Client 연결 종료 기본이 20분(IIS 기준) 확인   
#define CONFIG_HOST_NAME_VALUE						"test_00" 
#define CONFIG_NETWORK_BAND_WIDTH_VALUE				"1000000"// bps (1G)
#define CONFIG_STATIC_CORS_USE_VALUE				"0"
#define CONFIG_LOCAL_IP_VALUE						"127.0.0.1"
#define CONFIG_ETHERNET_VALUE						"eth0" //사용 안함 

