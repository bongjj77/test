//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "common/network/engine/tcp_network_object.h"

//====================================================================================================
// Interface
//====================================================================================================
class IHttpClient: public IObjectCallback
{
	
public:	

};

//====================================================================================================
// HttpClientObject
//================================================================================================on====
class HttpClientObject : public TcpNetworkObject 
{
public:
	HttpClientObject();
	virtual ~HttpClientObject();
	
public:
	bool			Create(TcpNetworkObjectParam *param);
	virtual void	Destroy();
	
	bool			SendPackt(int data_size, uint8_t *data);

	int				RecvHandler(std::shared_ptr<std::vector<uint8_t>>& data);
	bool			RecvPacketProcess(int data_size, uint8_t *data);
	
private:

};
