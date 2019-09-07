//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "../../common_header.h"
#include "tcp_tls_network_object.h"
#include "network_manager.h" 
#include <iostream>
#include <stdarg.h>

//====================================================================================================
// Constructor 
//====================================================================================================
TcpTlsNetworkObject::TcpTlsNetworkObject() 
{
	_socket_ssl = nullptr;	
}

//====================================================================================================
// Destructor 
//====================================================================================================
TcpTlsNetworkObject::~TcpTlsNetworkObject()
{
	if(_socket_ssl != nullptr)
	{
		_socket_ssl = nullptr; 
	}

	_network_callback 	= nullptr; 
	_object_callback = nullptr; 
}

//====================================================================================================
// Create  
//====================================================================================================
bool TcpTlsNetworkObject::Create(TcpTlsNetworkObjectParam* param)
{
	if(param->socket_ssl == nullptr)
	{
		return false; 
	}

	_socket_ssl = param->socket_ssl;
	_object_key = param->object_key;
	_object_name = param->object_name;

	try
	{
		_remote_ip_string = _socket_ssl->lowest_layer().remote_endpoint().address().to_v4().to_string();
		_remote_ip = boost::asio::detail::socket_ops::network_to_host_long(_socket_ssl->lowest_layer().remote_endpoint().address().to_v4().to_ulong()); 
		_remote_port = _socket_ssl->lowest_layer().remote_endpoint().port();		
	}
	catch (std::exception& error)
	{
		LOG_ERROR_WRITE(("[%s] TcpTlsNetworkObject::Create - Socket Exception - ip(%s)", 
						param->object_name, _remote_ip_string.c_str())); 

		return false;
	}	
 
	_network_callback	= param->network_callback;
	_object_callback	= param->object_callback;
	_create_time		= time(nullptr);

	return true;

}

//====================================================================================================
// IsOpened  
//====================================================================================================
bool TcpTlsNetworkObject::IsOpened()
{
	return _socket_ssl->lowest_layer().is_open();
}

//====================================================================================================
// SocketClose  
//====================================================================================================
void TcpTlsNetworkObject::SocketClose()
{
	_socket_ssl->lowest_layer().close();
}

//====================================================================================================
// GetIoContext  
//====================================================================================================
boost::asio::io_context& TcpTlsNetworkObject::GetIoContext()
{
	return (boost::asio::io_context&)_socket_ssl->get_executor().context();

};

//====================================================================================================
// AsyncRead  
//====================================================================================================
void TcpTlsNetworkObject::AsyncRead()
{
	_socket_ssl->async_read_some(boost::asio::buffer(*_recv_buffer),
		boost::bind(&TcpTlsNetworkObject::OnReceive,
			shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

//====================================================================================================
// AsyncWrite  
//====================================================================================================
void TcpTlsNetworkObject::AsyncWrite(std::shared_ptr<std::vector<uint8_t>> data)
{
	boost::asio::async_write(*_socket_ssl,
		boost::asio::buffer(*data),
		boost::bind(&TcpTlsNetworkObject::OnSend,
			shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}
