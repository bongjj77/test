//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "common/network/engine/tcp_network_manager.h"  
#include "test_tcp_server_object.h" 

//====================================================================================================
// TestTcpServerManager
//====================================================================================================
class TestTcpServerManager : public TcpNetworkManager
{
public:
	TestTcpServerManager(int object_key);

public : 
	bool ConnectedAdd(std::shared_ptr<NetTcpSocket> socket, 
					std::shared_ptr<ITestTcpServerCallback> callback, 
					int &index_key);

	bool SendEchoData(int index_key, int data_size, uint8_t * data);
private :

}; 
