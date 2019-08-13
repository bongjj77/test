//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "../common/network/engine/tcp_network_manager.h"  
#include "http_client_object.h" 

//====================================================================================================
// HttpClientManager
//====================================================================================================
class HttpClientManager : public TcpNetworkManager
{
public:
	HttpClientManager(int object_key);

public : 
	bool AcceptedAdd(std::shared_ptr<NetTcpSocket> socket, 
					uint32_t ip, 
					int port, 
					std::shared_ptr<IHttpClient> callback,
					int &index_key);
	
private : 
	
}; 
