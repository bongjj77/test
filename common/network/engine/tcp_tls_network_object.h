//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "tcp_network_object.h"

class TcpTlsNetworkObject;

//====================================================================================================
// Object Create Paam
//====================================================================================================
struct TcpTlsNetworkObjectParam
{
	int object_key;
	std::shared_ptr<IObjectCallback> object_callback;
	std::string object_name;
 
	std::shared_ptr<NetSocketSSL> socket_ssl;
	std::shared_ptr<INetworkCallback> network_callback;
};

//====================================================================================================
// TcpTlsNetworkObject(Session) 
//====================================================================================================
class TcpTlsNetworkObject : TcpNetworkObject
{
public:
	explicit TcpTlsNetworkObject();
	virtual ~TcpTlsNetworkObject();

public:
	virtual bool Create(TcpTlsNetworkObjectParam* param);
	virtual bool IsOpened();

protected :
 	// interface 
	virtual int RecvHandler(std::shared_ptr<std::vector<uint8_t>>& data) { return 0; }

	
	virtual void SocketClose();
	virtual boost::asio::io_context& GetIoContext();
	virtual void AsyncRead();
	virtual void AsyncWrite(std::shared_ptr<std::vector<uint8_t>> data);
	
protected : 
	
	std::shared_ptr<NetSocketSSL> _socket_ssl = nullptr;
  	
};

