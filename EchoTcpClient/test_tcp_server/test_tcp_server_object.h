//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "common/network/engine/tcp_network_object.h"
#include "common/network/protocol/test_protocol.h"

//====================================================================================================
// Interface
//====================================================================================================
class ITestTcpServerCallback : public IObjectCallback
{
	
public:
	

};

//====================================================================================================
// TestTcpServerObject
//================================================================================================on====
class TestTcpServerObject : public TcpNetworkObject 
{
public:
					TestTcpServerObject();
	virtual			~TestTcpServerObject();
	
public:
	bool			Create(TcpNetworkObjectParam * param);
	virtual void	Destroy();

	bool			SendPackt(PacketType type_code, int data_size, uint8_t *data);
	bool			SendEchoData(int data_size, uint8_t *data);

	int				RecvHandler(std::shared_ptr<std::vector<uint8_t>>& data);
	bool			RecvPacketProcess(PacketType type_code, int data_size, uint8_t *data);
	bool			RecvEchoResponse(int data_size, uint8_t *data);

private:

};
