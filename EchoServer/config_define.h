#ifndef __H_CONFIG_DEFINE_H__
#define __H_CONFIG_DEFINE_H__

#ifdef WIN32
#define DEFAULT_CONFIG_FILE									"StreamTester.conf"
#else
#define DEFAULT_CONFIG_FILE									"stream_tester.conf"    
#endif 

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
#define CONFIG_TRAFFIC_TEST						"traffic_test"

// Signalling 서버 정보 설정 
#define CONFIG_SIGNALLING_TYPE					"signalling_type"
#define CONFIG_SIGNALLING_ADDRESS				"signalling_address"
#define CONFIG_SIGNALLING_PORT					"signalling_port"
#define CONFIG_SIGNALLING_APP					"signalling_app"
#define CONFIG_SIGNALLING_STREAM				"signalling_stream"


#define CONFIG_SIGNALLING_STREAM				"signalling_stream"

// Streaming 생성 정보 설정 
#define CONFIG_STREAMING_START_COUNT			"streaming_start_count"		// 최초 생성 개수 
#define CONFIG_STREAMING_CREATE_INTERVAL		"streaming_create_interval"	// 생성 간격 (초) 
#define CONFIG_STREAMING_CREATE_COUNT			"streaming_create_count"	// 생성 개수 
#define CONFIG_STREAMING_MAX_COUNT				"streaming_max_count"		// 최대 생성 개수(0-무제한) 

//------------------------------ 기본  값 ------------------------------
#define CONFIG_THREAD_POOL_COUNT_VALUE				"0" // 0 - cpu 개수
#define CONFIG_FILE_SAVE_PATH_VALUE					"./" 
#define CONFIG_LOG_FILE_PATH_VALUE					"./log/stream_tester.log"
#define CONFIG_VERSION_VALUE						"V_2019"
#define CONFIG_DEBUG_MODE_VALUE						"0"
#define	CONFIG_SYS_LOG_BACKUP_HOUR_VALUE			"4"
#define CONFIG_KEEPALIVE_TIME_VALUE					"300" 
#define CONFIG_HTTP_CLIENT_KEEPALIVE_TIME_VALUE		"100"//Client 연결 종료 기본이 20분(IIS 기준) 확인   
#define CONFIG_HOST_NAME_VALUE						"tester_00" 
#define CONFIG_NETWORK_BAND_WIDTH_VALUE				"1000000"// bps (1G)
#define CONFIG_STATIC_CORS_USE_VALUE				"0"
#define CONFIG_LOCAL_IP_VALUE						"127.0.0.1"
#define CONFIG_ETHERNET_VALUE						"eth0" //사용 안함 

// Signalling 서버 정보 설정 
#define CONFIG_SIGNALLING_TYPE_VALUE				"0" // 0 - ome, 1 - wowza
#define CONFIG_SIGNALLING_ADDRESS_VALUE				"127.0.0.1"
#define CONFIG_SIGNALLING_PORT_VALUE				"3333"
#define CONFIG_SIGNALLING_APP_VALUE					"app"
#define CONFIG_SIGNALLING_STREAM_VALUE				"abc_o"

// Streaming 생성 정보 설정 
#define CONFIG_STREAMING_START_COUNT_VALUE			"10"	// 최초 생성 개수 
#define CONFIG_STREAMING_CREATE_INTERVAL_VALUE		"10"	// 생성 간격 (초) 
#define CONFIG_STREAMING_CREATE_COUNT_VALUE			"10"	// 생성 개수 
#define CONFIG_STREAMING_MAX_COUNT_VALUE			"50"// 최대 생성 개수(0-무제한)

#define CONFIG_TRAFFIC_TEST_VALUE					"0" // 1 - TestUdpServer 접속 처리

#endif // __H_CONFIG_DEFINE_H__
