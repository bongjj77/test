//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "../common/network/engine/tcp_network_manager.h"  
#include "test_tcp_client_object.h" 

//====================================================================================================
// TestTcpClintManager
//====================================================================================================
class TestTcpClintManager : public TcpNetworkManager
{
public:
	TestTcpClintManager(int object_key);

public : 
	bool AcceptedAdd(std::shared_ptr<NetTcpSocket> socket, 
					uint32_t ip, 
					int port, 
					ITestTcpClientCallback * callback, 
					int &index_key);
	
private : 
	
}; 
