﻿//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "common/thread_timer.h"
#include "common/network/engine/network_manager.h"
#include "test_tcp_client/test_tcp_client_manager.h"

enum class NetworkObjectKey
{
	TestTcpClient,
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
	int			test_tcp_client_listen_port;
	time_t		start_time;
};

//====================================================================================================
// MainObject
//====================================================================================================
class MainObject :	public std::enable_shared_from_this<MainObject>,
					public IThreadTimer, 
					public ITcpNetwork,
					public ITestTcpClient
{
public:
	MainObject(void);
	~MainObject(void);

public:
	bool Create(std::unique_ptr<CreateParam> create_param);
	void Destroy(void);
	bool RemoveNetwork(int object_key, int index_key); 
	bool RemoveNetwork(int object_key, std::vector<int> & index_keys); 
	
	static std::string GetNetworkObjectName(NetworkObjectKey object_key);
	
private:
	
	// ITcpNetwork Implement 
	bool OnTcpNetworkAccepted(int object_key, std::shared_ptr<NetTcpSocket> socket, uint32_t ip, int port);	
		
	bool OnTcpNetworkConnected(	int object_key,
								NetConnectedResult result,
								std::shared_ptr<std::vector<uint8_t>> connected_param,
								std::shared_ptr<NetTcpSocket> socket,
								unsigned ip,
								int port);	

	int  OnNetworkClose(int object_key, int index_key, uint32_t ip, int port); 	

	// Timer callback
	void OnThreadTimer(uint32_t timer_id, bool &delete_timer);
		
private:
	ThreadTimer								_timer;
	std::unique_ptr<CreateParam>			_create_param;
	std::shared_ptr<NetworkContextPool>		_network_context_pool = nullptr;	
	std::shared_ptr<NetworkManager>			_network_table[(int)NetworkObjectKey::Max];
	std::shared_ptr<TestTcpClintManager>	_test_tcp_client_manager;
};