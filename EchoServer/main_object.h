#pragma once
#include "./common/thread_timer.h"
#include "./common/network/engine/network_manager.h"
#include "stream_manager.h" 
#include <iostream>
#include <fstream>

enum class NetworkObjectKey
{
	EchoClient,

	Max,
};

//====================================================================================================
// Param
//====================================================================================================
struct CreateParam
{
	char 	version[MAX_PATH];
	bool	debug_mode;
	char 	host_name[MAX_PATH];
	char 	real_host_name[MAX_PATH];
	int		network_bandwidth; //Kbps
	uint32_t	local_ip;
	int		thread_pool_count;
};

//====================================================================================================
// MainObject
//====================================================================================================
class MainObject : 	public ITimerCallback, public INetworkCallback
{
public:
	MainObject(void);
	~MainObject(void);

public:
	std::string GetCurrentDateTime();
	bool Create(CreateParam *param);
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
	
	// INetworkCallback 구현 
	bool OnUdpNetworkConnected(int object_key, 
								NetConnectedResult result,
								char * connected_param, 
								NetUdpSocket * socket, 
								unsigned ip, 
								int port) { return true;  }	 
	int OnUdpNetworkClose(int object_key, int index_key, uint32_t ip, int port) { return 0;  }
	
	
	// Timer 
	void OnThreadTimer(uint32_t timer_id, bool &delete_timer);
		
private:
	ThreadTimer			_thread_timer;
	CreateParam			_create_param;
	std::shared_ptr<NetworkContextPool> _network_service_pool = nullptr;	
	NetworkManager		*_network_table[(int)NetworkObjectKey::Max];
	time_t				_start_time;


};