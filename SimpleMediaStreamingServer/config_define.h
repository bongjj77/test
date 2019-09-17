//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================
#pragma once
 
#define DEFAULT_CONFIG_FILE "media_streaming_server.conf"
 
//------------------------------ setting name ------------------------------
#define CONFIG_VERSION						"version"
#define CONFIG_THREAD_POOL_COUNT			"thread_pool_count"
#define CONFIG_DEBUG_MODE					"debug_mode"
#define	CONFIG_SYS_LOG_BACKUP_HOUR			"sys_log_backup_hour"
#define CONFIG_LOG_FILE_PATH				"log_file_path" 
#define CONFIG_HOST_NAME					"host_name"
#define CONFIG_NETWORK_BAND_WIDTH			"network_band_width"
#define CONFIG_LOCAL_IP						"local_ip"
#define CONFIG_RTMP_LISTEN_PORT				"rtmp_listen_port"
#define CONFIG_HTTP_LISTEN_PORT				"http_listen_port"

//------------------------------ default value ------------------------------
#define CONFIG_VERSION_VALUE				"V_2019"
#define CONFIG_THREAD_POOL_COUNT_VALUE		"0" // 0 - cpu count
#define CONFIG_DEBUG_MODE_VALUE				"0"
#define	CONFIG_SYS_LOG_BACKUP_HOUR_VALUE	"4"
#define CONFIG_LOG_FILE_PATH_VALUE			"./log/media_streaming_server.log"
#define CONFIG_HOST_NAME_VALUE				"test_00" 
#define CONFIG_NETWORK_BAND_WIDTH_VALUE		"1000000"// bps (1G)
#define CONFIG_LOCAL_IP_VALUE				"127.0.0.1"
#define CONFIG_RTMP_LISTEN_PORT_VALUE		"1935"
#define CONFIG_HTTP_LISTEN_PORT_VALUE		"8080"

