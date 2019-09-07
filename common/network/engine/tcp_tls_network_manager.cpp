//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "tcp_tls_network_manager.h"

#include "root_certificates.hpp"

#ifndef WIN32
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/time.h>
#endif

//====================================================================================================
// Constructor 
//====================================================================================================
TcpTlsNetworkManager::TcpTlsNetworkManager(int object_key) : TcpNetworkManager(object_key)
{
	_ssl_context = nullptr;
	_accept_socket_ssl = nullptr;
}

//====================================================================================================
// Destructor 
//====================================================================================================
TcpTlsNetworkManager::~TcpTlsNetworkManager( )
{
	LOG_WRITE(("[%s] TcpTlsNetworkManager::~TcpTlsNetworkManager()", _object_name.c_str()));
		 
	if(_accept_socket_ssl != nullptr)
	{
		_accept_socket_ssl = nullptr; 
	}	
 
}

//====================================================================================================
// 종료  
//====================================================================================================
void TcpTlsNetworkManager::Release() 
{
	TcpNetworkManager::Release();

	if(_accept_socket_ssl != nullptr)
	{
		//if(_accept_socket_ssl->lowest_layer().is_open() == true)
		{
			_accept_socket_ssl->lowest_layer().close(); 
		}
	}	
}
 
//====================================================================================================
// SSL 생성 
// - private_accepter_service : 연결 개수 가 많은 Network에서 사용(전용 Service 할당)  
//====================================================================================================
bool TcpTlsNetworkManager::CreateSSL(std::shared_ptr<ITlsTcpNetwork> callback,
									std::shared_ptr<NetworkContextPool>service_pool,
									std::string cert_file,
									std::string key_file,
									std::string verify_file,	
									int listen_port, 
									std::string object_name, 
									bool private_accepter_service) 
{
	bool result = false; 

	// server
	if (listen_port != 0)
	{
		_ssl_context = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);

		if (!cert_file.empty())
			_ssl_context->use_certificate_chain_file(cert_file);

		if (!key_file.empty())
			_ssl_context->use_private_key_file(key_file, boost::asio::ssl::context::pem);

		if (!verify_file.empty())
		{
			_ssl_context->load_verify_file(verify_file);
			_ssl_context->set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert | boost::asio::ssl::verify_client_once);
		}

	}
	// client 
	else
	{
		_ssl_context = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);
	
		// This holds the root certificate used for verification
		load_root_certificates(*_ssl_context);

		// Verify the remote server's certificate
		_ssl_context->set_verify_mode(boost::asio::ssl::verify_peer);
	}
		
	result = Create(std::static_pointer_cast<ITcpNetwork>(callback), service_pool, listen_port, object_name, private_accepter_service);

	return result;
}
 
//====================================================================================================
// 연결(비동기 방식) 
//====================================================================================================
bool TcpTlsNetworkManager::PostConnect(uint32_t ip, std::string host, int port, std::shared_ptr < std::vector<uint8_t>> connected_param)
{
	std::string ip_string;

	//Network Address -> host Address
	ip = boost::asio::detail::socket_ops::network_to_host_long(ip);
	ip_string = NetAddress_v4(ip).to_string();
	
	auto socket = std::make_shared<NetTlsSocket>(*(_context_pool->GetContext()), *_ssl_context);
	
	socket->set_verify_callback(
		boost::bind(&TcpTlsNetworkManager::VerifyCallback, this, _1, _2));

	/*
	if (!SSL_set_tlsext_host_name(socket->native_handle(), host.c_str()))
	{
		return false;;
	}
	*/

	boost::asio::ip::tcp::endpoint endPoint(boost::asio::ip::address::from_string(ip_string.c_str()), port);

	socket->lowest_layer().async_connect(endPoint, boost::bind(&TcpTlsNetworkManager::OnConnectedSSL, 
																this, 
																boost::asio::placeholders::error, 
																socket, 
																connected_param, 
																inet_addr(ip_string.c_str()), port));

	return true;
}

bool TcpTlsNetworkManager::VerifyCallback(bool preverified, boost::asio::ssl::verify_context& ctx)
{
	// The verify callback can be used to check whether the certificate that is
	// being presented is valid for the peer. For example, RFC 2818 describes
	// the steps involved in doing this for HTTPS. Consult the OpenSSL
	// documentation for more details. Note that the callback is called once
	// for each certificate in the certificate chain, starting from the root
	// certificate authority.
	char subject_name[256];
	X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
	X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
	std::cout << "Verifying " << subject_name << "\n";

	return true;
}
 
//====================================================================================================
// Connected 결과   
//====================================================================================================
void TcpTlsNetworkManager::OnConnectedSSL(	const NetErrorCode & error, 
										std::shared_ptr<NetTlsSocket> socket, 
										std::shared_ptr<std::vector<uint8_t>> connected_param, 
										uint32_t ip, 
										int port)
{
	NetConnectedResult result = NetConnectedResult::Success;

	// 에러 
	if (error)
	{
		LOG_ERROR_WRITE(("[%s] TcpTlsNetworkManager::OnConnectedSSL - ip(%s) Port(%d) Error(%d) Message(%s) ",
			_object_name.c_str(),
			GetStringIP(ip).c_str(),
			port,
			error.value(),
			error.message().c_str()));


		if (socket != nullptr)
		{
			socket->lowest_layer().close();
			socket = nullptr;
		}


		result = NetConnectedResult::Fail;

	}
	else
	{
		result = NetConnectedResult::Success;

	}
	
	socket->async_handshake( boost::asio::ssl::stream_base::client, 
							boost::bind(&TcpTlsNetworkManager::OnHandshakeSSL,
								this,
								boost::asio::placeholders::error,
								socket,
								connected_param,
								ip, 
								port));	
}

//====================================================================================================
// SSL HandShake 핸들러(client) 
//====================================================================================================
void TcpTlsNetworkManager::OnHandshakeSSL(	const NetErrorCode & error, 
										std::shared_ptr<NetTlsSocket> socket, 
										std::shared_ptr<std::vector<uint8_t>> connected_param, 
										uint32_t ip, 
										int port)
{
	NetConnectedResult result = NetConnectedResult::Success;

	// 에러 
	if (error)
	{
		LOG_ERROR_WRITE(("[%s] TcpTlsNetworkManager::OnHandshakeSSL - ip(%s) Port(%d) Error(%d) Message(%s) ",
			_object_name.c_str(),
			GetStringIP(ip).c_str(),
			port,
			error.value(),
			error.message().c_str()));


		if (socket != nullptr)
		{
			socket->lowest_layer().close();
			socket = nullptr;
		}


		result = NetConnectedResult::Fail;

	}
	else
	{
		result = NetConnectedResult::Success;

	}

	
	// 연결 콜백 호출 
	if (std::static_pointer_cast<ITlsTcpNetwork>(_network_callback)->OnTlsTcpNetworkConnected(_object_key, result, connected_param, socket, ip, port) == false)
	{
		LOG_ERROR_WRITE(("[%s] TcpTlsNetworkManager::OnHandshakeSSL - OnTcpNetworkConnected fail - ip(%s) Port(%d)",
			_object_name.c_str(),
			GetStringIP(ip).c_str(),
			port));
	}
}


//====================================================================================================
// Accept 이벤트 설정  
//====================================================================================================
bool TcpTlsNetworkManager::PostAccept()
{
	if(_acceptor == nullptr)
	{
		try
		{
			NetTcpEndPoint endpoint(boost::asio::ip::tcp::v4(), _listen_port);
			
			_acceptor = std::make_shared<NetAcceptor>(_is_private_accepter_service == true ? 
													*(_context_pool->GetPrivateContext()) : 
													*(_context_pool->GetContext()));
			
			_acceptor->open(endpoint.protocol());
			_acceptor->set_option( boost::asio::ip::tcp::acceptor::reuse_address(false));
			_acceptor->bind(endpoint);
			_acceptor->listen();
	
		}
		catch (const std::exception& e)
		{
			LOG_ERROR_WRITE(("[%s] TcpTlsNetworkManager::PostAccept - Accept Ready(Listen) fail - Error(%s)", 
				_object_name.c_str(), e.what()));
			return false; 
		}
	}	
 
	if(_accept_socket_ssl != nullptr)
	{
		LOG_WARNING_WRITE(("[%s] TcpTlsNetworkManager::PostAccept - _accept_socket_ssl not nullptr",
			_object_name.c_str()));
			
		//if(_accept_socket_ssl->is_open() == true)
		{
			_accept_socket_ssl->lowest_layer().close();
		}
		_accept_socket_ssl = nullptr; 
	}
		
	_accept_socket_ssl = std::make_shared<NetTlsSocket>(*(_context_pool->GetContext()), *_ssl_context);

	_acceptor->async_accept(_accept_socket_ssl->lowest_layer(), boost::bind(&TcpTlsNetworkManager::OnAccept, this, boost::asio::placeholders::error));
		
	return true; 

}

//====================================================================================================
// SSL Accept Handler 
//====================================================================================================
void TcpTlsNetworkManager::OnAccept(const NetErrorCode & error)
{
	// 에러 
	if(error)
	{
		LOG_ERROR_WRITE(("[%s] TcpTlsNetworkManager::OnSSLAccept - Error(%d) Message(%s)", 
				_object_name.c_str(), error.value(), error.message().c_str()));
				
		if(_accept_socket_ssl != nullptr)
		{
			//if(_accept_socket_ssl->lowest_layer().is_open() == true)
			{
				_accept_socket_ssl->lowest_layer().close(); 
			}
			_accept_socket_ssl 		= nullptr;
		}

		if(_is_closeing == false)
		{
			PostAccept();
		}
		
		return; 
	}

	if(_is_closeing == true)
	{
		return; 
	}
	
	boost::asio::ip::tcp::no_delay option(true); 
	_accept_socket_ssl->lowest_layer().set_option(option);
 	_accept_socket_ssl->async_handshake(boost::asio::ssl::stream_base::server, boost::bind(&TcpTlsNetworkManager::OnHandshakeSSL, this, boost::asio::placeholders::error));
	
	return; 
}


//====================================================================================================
// SSL HandShake Handler(accepter)
//====================================================================================================
void TcpTlsNetworkManager::OnHandshakeSSL(const NetErrorCode & error)
{
	uint32_t	ip 		= 0; 
	int 		port	= 0; 
		
	// 에러 
	if(error)
	{
		LOG_ERROR_WRITE(("[%s] TcpTlsNetworkManager::OnHandshakeSSL - Error(%d) Message(%s)", 
				_object_name.c_str(), error.value(), error.message().c_str()));
				
		if(_accept_socket_ssl != nullptr)
		{
			//if(_accept_socket_ssl->lowest_layer().is_open() == true)
			{
				_accept_socket_ssl->lowest_layer().close(); 
			}
			_accept_socket_ssl = nullptr;
		}

		if(_is_closeing == false)
		{
			PostAccept();
		}
		
		return; 
	}

	if(_is_closeing == true)
	{
		return; 
	}
		
	// IP/Port Setting 
	try
	{
		ip 	= boost::asio::detail::socket_ops::network_to_host_long(_accept_socket_ssl->lowest_layer().remote_endpoint().address().to_v4().to_ulong()); 
		port	= _accept_socket_ssl->lowest_layer().remote_endpoint().port();	
	}	
	catch (std::exception& error)
	{
		LOG_ERROR_WRITE(("[%s] TcpTlsNetworkManager::OnHandshakeSSL - Socket Exception", _object_name.c_str()));
		
		if(_accept_socket_ssl != nullptr)
		{
			//if(_accept_socket_ssl->lowest_layer().is_open() == true)
			{
				_accept_socket_ssl->lowest_layer().close(); 
			}

			_accept_socket_ssl = nullptr;
		}

		if(_is_closeing == false)
		{
			PostAccept();
		}
		
		return;
	}

	if(std::static_pointer_cast<ITlsTcpNetwork>(_network_callback)->OnTlsTcpNetworkAccepted(_object_key, _accept_socket_ssl, ip, port) == false)
	{
		LOG_ERROR_WRITE(("[%s] TcpTlsNetworkManager::OnHandshakeSSL -  OnTlsTcpNetworkAccepted return false", 
				_object_name.c_str()));
		
		if(_accept_socket_ssl != nullptr)
		{
			LOG_ERROR_WRITE(("[%s] TcpTlsNetworkManager::OnHandshakeSSL - Socket Not nullptr", _object_name.c_str()));
			
			//if(_accept_socket_ssl->lowest_layer().is_open() == true)
			{
				_accept_socket_ssl->lowest_layer().close(); 
			}
			_accept_socket_ssl = nullptr; 
		}
	}

	_accept_socket_ssl = nullptr; 
	
	PostAccept();
		
	return;
}
 
 