//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "./common/network/engine/tcp_network_object.h"
#include "test_protocol.h"

//====================================================================================================
// Interface
//====================================================================================================
class ITestTcpClientCallback
{
	
public:
	

};

//====================================================================================================
// TestTcpClientObject
//================================================================================================on====
class TestTcpClientObject : public TcpNetworkObject 
{
public:
					TestTcpClientObject();
	virtual			~TestTcpClientObject();
	
public:
	bool			Create(TcpNetworkObjectParam *param);
	virtual void	Destroy();
	
	bool			SendPackt(PacketType type_code, int data_size, uint8_t *data);

	int				RecvHandler(std::shared_ptr<std::vector<uint8_t>>& data);
	bool			RecvPacketProcess(PacketType type_code, int data_size, uint8_t *data);
	bool			RecvEchoRequest(int data_size, uint8_t *data);
	bool			RecvStreamRequest(int data_size, uint8_t *data);
private:

	


};
