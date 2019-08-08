﻿//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "common/thread_timer.h"
#include "common/network/engine/network_manager.h"
#include "stream_manager.h" 
#include "./test_tcp_server/test_tcp_server_manager.h"

enum class NetworkObjectKey
{
	TestTcpServer,
	Max,
};

//====================================================================================================
// Create Param
//====================================================================================================
struct CreateParam
{
	std::string version;
	int			thread_pool_count;
	bool		debug_mode;
	std::string host_name;
	std::string real_host_name;
	int			network_bandwidth; //Kbps
	uint32_t	local_ip;
	uint32_t	test_tcp_server_ip;
	int			test_tcp_server_port;
	time_t		start_time;
};

//====================================================================================================
// TestTcpServer Connected Param
//====================================================================================================
struct TestTcpServerConnectedParam
{
	int index;
};

//====================================================================================================
// MainObject
//====================================================================================================
class MainObject : public std::enable_shared_from_this<MainObject>,
				public ITimerCallback, 
				public INetworkCallback,
				public ITestTcpServerCallback
{
public:
	MainObject(void);
	~MainObject(void);

public:
	std::string GetCurrentDateTime();
	bool Create(std::unique_ptr<CreateParam> create_param);
	void Destroy(void);
	bool RemoveNetwork(int object_key, int index_key); 
	bool RemoveNetwork(int object_key, std::vector<int> & index_keys); 
	
	static std::string GetNetworkObjectName(NetworkObjectKey object_key);
	std::shared_ptr<StreamManager> &GetStreamManager(int stream_key);
	
private:
	
	// INetworkCallback 구현 
	bool OnTcpNetworkAccepted(int object_key, NetTcpSocket * & socket, uint32_t ip, int port);	
	
	bool OnTcpNetworkAcceptedSSL(int object_key, NetSocketSSL * & socket_ssl, uint32_t ip, int port) { return true; }
	
	bool OnTcpNetworkConnected(int object_key,
							NetConnectedResult result,
							std::shared_ptr<std::vector<uint8_t>> connected_param,
							NetTcpSocket * socket,
							unsigned ip,
							int port);
	
	bool OnTcpNetworkConnectedSSL(int object_key,
								NetConnectedResult result,
								std::shared_ptr<std::vector<uint8_t>> connected_param,
								NetSocketSSL *socket,
								unsigned ip,
								int port);

	int  OnNetworkClose(int object_key, int index_key, uint32_t ip, int port); 	

	bool OnUdpNetworkConnected(int object_key, NetConnectedResult result, char * connected_param, NetUdpSocket * socket, unsigned ip, int port) { return true;  }	 
	
	int OnUdpNetworkClose(int object_key, int index_key, uint32_t ip, int port) { return 0;  }
	
	
	// Connect Proc
	bool TestTcpServerConnectedProc(NetConnectedResult result_code,
									TestTcpServerConnectedParam *onnected_param, 
									NetTcpSocket *socket, 
									uint32_t ip, 
									int port);

	// Timer 
	void OnThreadTimer(uint32_t timer_id, bool &delete_timer);
	void StartTestTimeProc();


private:
	ThreadTimer							_thread_timer;
	std::unique_ptr<CreateParam>		_create_param;
	std::shared_ptr<NetworkContextPool> _network_service_pool = nullptr;	
	NetworkManager						*_network_table[(int)NetworkObjectKey::Max];
	TestTcpServerManager				_test_tcp_server_manager;
};