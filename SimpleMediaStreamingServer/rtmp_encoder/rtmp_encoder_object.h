//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "common/network/engine/tcp_network_object.h"

//====================================================================================================
// Interface
//====================================================================================================
class IRtmpEncoder : public IObjectCallback
{
	
public:	

};

//====================================================================================================
// RtmpEncoderObject
//================================================================================================on====
class RtmpEncoderObject : public TcpNetworkObject 
{
public:
					RtmpEncoderObject();
	virtual			~RtmpEncoderObject();
	
public:
	bool			Create(TcpNetworkObjectParam *param);
	virtual void	Destroy();
	
	bool			SendPackt(int data_size, uint8_t *data);

	int				RecvHandler(std::shared_ptr<std::vector<uint8_t>>& data);
	bool			RecvPacketProcess(int data_size, uint8_t *data);
	
private:

};
