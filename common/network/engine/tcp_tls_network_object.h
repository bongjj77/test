//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "tcp_network_object.h"
#include <boost/asio/ssl.hpp>
#include <openssl/ssl.h>

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket>  NetTlsSocket;

//====================================================================================================
// Network Interface 
//====================================================================================================
class ITlsTcpNetwork : public ITcpNetwork
{
public:
	virtual bool OnTlsTcpNetworkAccepted(int object_key, std::shared_ptr<NetTlsSocket>  socket_ssl, uint32_t ip, int port) = 0;	// Accepted(SSL)

	virtual bool OnTlsTcpNetworkConnected(int object_key,
										NetConnectedResult result,
										std::shared_ptr<std::vector<uint8_t>> connected_param,
										std::shared_ptr<NetTlsSocket> socket,
										unsigned ip,
										int port) = 0;
};

class TcpTlsNetworkObject;

//====================================================================================================
// Object Create Paam
//====================================================================================================
struct TcpTlsNetworkObjectParam
{
	int object_key;
	std::shared_ptr<IObjectCallback> object_callback;
	std::string object_name;
 
	std::shared_ptr<NetTlsSocket> socket_ssl;
	std::shared_ptr<ITlsTcpNetwork> network_callback;
};

//====================================================================================================
// TcpTlsNetworkObject(Session) 
//====================================================================================================
class TcpTlsNetworkObject : public TcpNetworkObject
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
	
	std::shared_ptr<NetTlsSocket> _socket_ssl = nullptr;
  	
};

