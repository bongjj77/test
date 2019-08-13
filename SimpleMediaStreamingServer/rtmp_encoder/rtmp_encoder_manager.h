//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "../common/network/engine/tcp_network_manager.h"  
#include "rtmp_encoder_object.h" 

//====================================================================================================
// RtmpEncoderManager
//====================================================================================================
class RtmpEncoderManager : public TcpNetworkManager
{
public:
	RtmpEncoderManager(int object_key);

public : 
	bool AcceptedAdd(std::shared_ptr<NetTcpSocket> socket, 
					uint32_t ip, 
					int port, 
					std::shared_ptr<IRtmpEncoder> callback, 
					int &index_key);
	
private : 
	
}; 
