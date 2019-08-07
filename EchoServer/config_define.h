//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================
#pragma once
 
#define DEFAULT_CONFIG_FILE "echo_server.conf"
 
//------------------------------ 설정 이름 ------------------------------
#define CONFIG_VERSION							"version"
#define CONFIG_THREAD_POOL_COUNT				"thread_pool_count"
#define CONFIG_DEBUG_MODE						"debug_mode"
#define	CONFIG_SYS_LOG_BACKUP_HOUR				"sys_log_backup_hour"
#define CONFIG_LOG_FILE_PATH					"log_file_path" 
#define CONFIG_HOST_NAME						"host_name"
#define CONFIG_NETWORK_BAND_WIDTH				"network_band_width"
#define CONFIG_LOCAL_IP							"local_ip"
#define CONFIG_TEST_TCP_CLIENT_LISTEN_PORT		"test_tcp_client_listen_port"

//------------------------------ 기본  값 ------------------------------
#define CONFIG_VERSION_VALUE						"V_2019"
#define CONFIG_THREAD_POOL_COUNT_VALUE				"0" // 0 - cpu count
#define CONFIG_FILE_SAVE_PATH_VALUE					"./" 
#define CONFIG_DEBUG_MODE_VALUE						"0"
#define	CONFIG_SYS_LOG_BACKUP_HOUR_VALUE			"4"
#define CONFIG_LOG_FILE_PATH_VALUE					"./log/echo_server.log"
#define CONFIG_HOST_NAME_VALUE						"test_00" 
#define CONFIG_NETWORK_BAND_WIDTH_VALUE				"1000000"// bps (1G)
#define CONFIG_LOCAL_IP_VALUE						"127.0.0.1"
#define CONFIG_TEST_TCP_CLIENT_LISTEN_PORT_VALUE	"5555"

