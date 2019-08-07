//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "tcp_network_manager.h"

#include "root_certificates.hpp"

#ifndef WIN32
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/time.h>
#endif


#define INDEX_KEY_MAX_SIZE (2000000000) //-2147483648 ~ 2147483647


//====================================================================================================
// Constructor 
//====================================================================================================
TcpNetworkManager::TcpNetworkManager(int object_key) : NetworkManager(object_key)
{
	
	_index_key 				= 0;
	_max_createing_count 		= 0;
	_network_info_map.clear();
	_listen_port				= 0; 
	_is_private_accepter_service 	= false; 
	_is_closeing					= false; 
	_release_timer				= nullptr;
	_acceptor					= nullptr;
	_accept_socket				= nullptr; 
	_network_info_map.clear();


	_is_support_ssl				= false;
	_ssl_context				= nullptr;
	_timeout_request			= 0;
	_timeout_content			= 0;
	_accept_socket_ssl			= nullptr;

}


//====================================================================================================
// Destructor 
//====================================================================================================
TcpNetworkManager::~TcpNetworkManager( )
{
	LOG_WRITE(("[%s] TcpNetworkManager::~TcpNetworkManager()", _object_name.c_str()));

	if(_release_timer != nullptr)
	{
		delete _release_timer;
		_release_timer = nullptr; 
	}

	if(_accept_socket != nullptr)
	{
		delete _accept_socket;
		_accept_socket = nullptr; 
	}
	
	if(_accept_socket_ssl != nullptr)
	{
		delete _accept_socket_ssl;
		_accept_socket_ssl = nullptr; 
	}	
 
}

//====================================================================================================
// 종료 
// - Linux에서 동일 Thread에서 종료 시켜야 정상 처리 예상 
//====================================================================================================
void TcpNetworkManager::PostRelease()
{
	
	LOG_WRITE(("[%s] TcpNetworkManager::PostRelease()", _object_name.c_str()));

	_is_closeing = true; 
	
#ifdef _WIN32
	Release();
#else
	if(_acceptor != nullptr)
	{
		if(_release_timer != nullptr)
		{
			return; 
		}
			
		_release_timer = new NetTimer((boost::asio::io_context&)_acceptor->get_executor().context());
		_release_timer->expires_from_now(std::chrono::nanoseconds(1000000));
		_release_timer->async_wait(boost::bind(&TcpNetworkManager::Release, this ));
	}
	else
	{
		Release(); 
	}
#endif 
	
}

//====================================================================================================
// 종료  
//====================================================================================================
void TcpNetworkManager::Release() 
{
	
	LOG_WRITE(("[%s] TcpNetworkManager::Release()", _object_name.c_str()));

	RemoveAll(); 
		
	//Accepter 정리 
	if(_acceptor != nullptr)
	{
		//if(_acceptor->is_open() == true)
		{
			_acceptor->close(); 
		}

		delete _acceptor;
		_acceptor = nullptr; 
	}
	
	if(_release_timer != nullptr)
	{
		_release_timer->cancel();
	}
	

	if(_accept_socket != nullptr)
	{
		//if(_accept_socket->is_open() == true)
		{
			_accept_socket->close(); 
		}
	}


	if(_accept_socket_ssl != nullptr)
	{
		//if(_accept_socket_ssl->lowest_layer().is_open() == true)
		{
			_accept_socket_ssl->lowest_layer().close(); 
		}
	}	


}
 
//====================================================================================================
// 생성 
// - private_accepter_service : 연결 개수 가 많은 Network에서 사용(전용 Service 할당)  
//====================================================================================================
bool TcpNetworkManager::Create(INetworkCallback * callback, 
								std::shared_ptr<NetworkContextPool> service_pool, 
								int listen_port, 
								std::string object_name, 
								bool private_accepter_service/*=false*/)
{
	_object_name = object_name; 	
	_network_callback = callback; 	
	_context_pool = service_pool;

	//서버 접속 생성 
	if(listen_port != 0)
	{
		_listen_port = listen_port; 
		_is_private_accepter_service = private_accepter_service;
		if(PostAccept() == false)
		{
			return false;
		}
	}

	return true;
}

//====================================================================================================
// SSL 생성 
// - private_accepter_service : 연결 개수 가 많은 Network에서 사용(전용 Service 할당)  
//====================================================================================================
bool TcpNetworkManager::CreateSSL(	INetworkCallback	*callback,
									std::shared_ptr<NetworkContextPool>service_pool,
									std::string 		cert_file,
									std::string			key_file,
									std::string			verify_file,	
									int 				listen_port, 
									std::string			object_name, 
									bool 				private_accepter_service,
									int					timeout_request, 
									long 				timeout_content) 
{
	bool result = false;
	
	// SSL 설정
	_is_support_ssl = true;

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
	
	_timeout_request = timeout_request;
	_timeout_content = timeout_content;
	
	result = Create(callback, service_pool, listen_port, object_name, private_accepter_service);

	return result;
}


//====================================================================================================
//삽입 
// - Key가 존재하는지 확인 
// - return -1 실패 
// - return n (IndexKey) 
//====================================================================================================
int TcpNetworkManager::Insert(std::shared_ptr<TcpNetworkObject> object, bool bKeepAliveCheck/* = false*/, uint32_t keepalive_check_time /*= 0*/)
{
	int index_key	= -1; 

	if(object == nullptr)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::Insert() Parameter nullptr", _object_name.c_str()));
		return -1;
	}

	
	std::lock_guard<std::mutex> network_info_map_lock(_network_info_map_mutex);;

	if(_index_key >= INDEX_KEY_MAX_SIZE)
	{
		_index_key = 0; 
	}

	// 인덱스키가 이미 사용 중이면 다음 인덱스 키 검색 
	if(_network_info_map.find(_index_key) != _network_info_map.end())
	{
		bool bFindIndex = false; 

		for(int index = _index_key; index < INDEX_KEY_MAX_SIZE ; index++)
		{
			if(_network_info_map.find(index) == _network_info_map.end())
			{
				_index_key = index;
				bFindIndex = true;
				break; 
			}
		}

		if(bFindIndex == false)
		{
			LOG_WRITE(("ERROR : [%s] TcpNetworkManager::Insert() IndexKey Over", _object_name.c_str()));
		}
	}

	// 삽입  
	if(_network_info_map.find(_index_key) == _network_info_map.end())
	{
		_network_info_map.insert(std::pair<int, std::shared_ptr<TcpNetworkObject>>(_index_key, object));

		// IndexKey 설정 
		index_key = _index_key; 

		// TcpNetworkObject에 IndexKey설정 
		object->SetIndexKey(_index_key);
		
		
		// 최대 개수 기록 
		if(_network_info_map.size() > _max_createing_count)
		{
			_max_createing_count = (uint32_t)_network_info_map.size();
		}
		
		// Object 통신 시작 
		if(object->Start() == false)
		{
			LOG_WRITE(("ERROR : [%s] TcpNetworkManager::Insert() Object Start Fail", _object_name.c_str()));
			index_key = -1;
		}
	
		// KeepAlive체크 설정 
		if(bKeepAliveCheck == true && keepalive_check_time != 0 && index_key != -1)
		{
			object->StartKeepAliveCheck(keepalive_check_time);
		}	

		//Open  확인 
		if(object->IsOpened() == false)
		{
			LOG_WRITE(("ERROR : [%s] TcpNetworkManager::Insert() Object Socket Close", _object_name.c_str()));
			index_key = -1; 
		}


		// 에러 처리 
		if(index_key == -1)
		{
			// 종료 요청 
			object->PostClose();

			// 삭제 
			_network_info_map.erase(_index_key);
		}
		
		// Index Key 증가 
		_index_key++; 
			
	}
	else 
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::Insert() Exist IndexKey", _object_name.c_str()));
		index_key = -1; 
	}

	return index_key;
}

//====================================================================================================
// 연결(비동기 방식) 
//====================================================================================================
bool TcpNetworkManager::PostConnect(uint32_t ip, int port, std::shared_ptr<std::vector<uint8_t>> connected_param)
{
	std::string ip_string;
		
	//Network Address -> host Address
	ip = boost::asio::detail::socket_ops::network_to_host_long(ip);	
	ip_string = NetAddress_v4(ip).to_string();

	return PostConnect(ip_string, port, connected_param); 
}

//====================================================================================================
// 연결(비동기 방식) 
//====================================================================================================
bool TcpNetworkManager::PostConnect(std::string & ip_string, int port, std::shared_ptr < std::vector<uint8_t>> connected_param)
{
	auto * socket = new NetTcpSocket(*(_context_pool->GetContext())); 

	boost::asio::ip::tcp::endpoint endPoint(boost::asio::ip::address::from_string(ip_string.c_str()), port);
	
	socket->async_connect(endPoint, boost::bind(&TcpNetworkManager::OnConnected, 
												this, 
												boost::asio::placeholders::error, 
												socket, 
												connected_param, 
												inet_addr(ip_string.c_str()), port));

	return true;
}

//====================================================================================================
// 연결(비동기 방식) 
//====================================================================================================
bool TcpNetworkManager::PostConnectSSL(uint32_t ip, std::string host, int port, std::shared_ptr < std::vector<uint8_t>> connected_param)
{
	std::string ip_string;

	//Network Address -> host Address
	ip = boost::asio::detail::socket_ops::network_to_host_long(ip);
	ip_string = NetAddress_v4(ip).to_string();


	auto * socket = new NetSocketSSL(*(_context_pool->GetContext()), *_ssl_context);
	
	socket->set_verify_callback(
		boost::bind(&TcpNetworkManager::VerifyCallback, this, _1, _2));

	/*
	if (!SSL_set_tlsext_host_name(socket->native_handle(), host.c_str()))
	{
		return false;;
	}
	*/

	boost::asio::ip::tcp::endpoint endPoint(boost::asio::ip::address::from_string(ip_string.c_str()), port);

	socket->lowest_layer().async_connect(endPoint, boost::bind(&TcpNetworkManager::OnConnectedSSL, 
																this, 
																boost::asio::placeholders::error, 
																socket, 
																connected_param, 
																inet_addr(ip_string.c_str()), port));

	return true;
}

bool TcpNetworkManager::VerifyCallback(bool preverified, boost::asio::ssl::verify_context& ctx)
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
void TcpNetworkManager::OnConnected(const NetErrorCode & error, NetTcpSocket * socket, std::shared_ptr<std::vector<uint8_t>> connected_param, uint32_t ip, int port)
{
	NetConnectedResult result= NetConnectedResult::Success;
				
	// 에러 
	if(error)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::OnConnected - IP(%s) Port(%d) Error(%d) Message(%s) ", 
			_object_name.c_str(), 
			GetStringIP(ip).c_str(), 
			port, 
			error.value(),
			error.message().c_str()));

		
		if(socket != nullptr)
		{
			socket->close();

			delete socket; 
			socket = nullptr; 
		}

		
		result = NetConnectedResult::Fail;
		
	}
	else 
	{
		result = NetConnectedResult::Success;

	}			
	
	// 연결 콜백 호출 
	if(_network_callback->OnTcpNetworkConnected(_object_key, result, connected_param, socket, ip, port) == false)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::OnConnected - OnTcpNetworkConnected return false - IP(%s) Port(%d)", 
			_object_name.c_str(), 
			GetStringIP(ip).c_str(),
			port));
	}
}

//====================================================================================================
// Connected 결과   
//====================================================================================================
void TcpNetworkManager::OnConnectedSSL(const NetErrorCode & error, NetSocketSSL * socket, std::shared_ptr<std::vector<uint8_t>> connected_param, uint32_t ip, int port)
{
	NetConnectedResult result = NetConnectedResult::Success;

	// 에러 
	if (error)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::OnConnectedSSL - IP(%s) Port(%d) Error(%d) Message(%s) ",
			_object_name.c_str(),
			GetStringIP(ip).c_str(),
			port,
			error.value(),
			error.message().c_str()));


		if (socket != nullptr)
		{
			socket->lowest_layer().close();

			delete socket;
			socket = nullptr;
		}


		result = NetConnectedResult::Fail;

	}
	else
	{
		result = NetConnectedResult::Success;

	}

	
	socket->async_handshake( boost::asio::ssl::stream_base::client, 
							boost::bind(&TcpNetworkManager::OnHandshakeSSL,
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
void TcpNetworkManager::OnHandshakeSSL(const NetErrorCode & error, NetSocketSSL * socket, std::shared_ptr<std::vector<uint8_t>> connected_param, uint32_t ip, int port)
{
	NetConnectedResult result = NetConnectedResult::Success;

	// 에러 
	if (error)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::OnHandshakeSSL - IP(%s) Port(%d) Error(%d) Message(%s) ",
			_object_name.c_str(),
			GetStringIP(ip).c_str(),
			port,
			error.value(),
			error.message().c_str()));


		if (socket != nullptr)
		{
			socket->lowest_layer().close();

			delete socket;
			socket = nullptr;
		}


		result = NetConnectedResult::Fail;

	}
	else
	{
		result = NetConnectedResult::Success;

	}

	
	// 연결 콜백 호출 
	if (_network_callback->OnTcpNetworkConnectedSSL(_object_key, result, connected_param, socket, ip, port) == false)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::OnHandshakeSSL - OnTcpNetworkConnected fail - IP(%s) Port(%d)",
			_object_name.c_str(),
			GetStringIP(ip).c_str(),
			port));
	}
}


//====================================================================================================
// Accept 이벤트 설정  
//====================================================================================================
bool TcpNetworkManager::PostAccept()
{
	if(_acceptor == nullptr)
	{
		try
		{
			NetTcpEndPoint endpoint(boost::asio::ip::tcp::v4(), _listen_port);
			
			_acceptor = new NetAcceptor(_is_private_accepter_service == true ? *(_context_pool->GetPrivateContext()) : *(_context_pool->GetContext()));
			
			_acceptor->open(endpoint.protocol());
			_acceptor->set_option( boost::asio::ip::tcp::acceptor::reuse_address(false));
			_acceptor->bind(endpoint);
			_acceptor->listen();
	
		}
		catch (const std::exception& e)
		{
			LOG_WRITE(("ERROR : [%s] TcpNetworkManager::PostAccept - Accept Ready(Listen) fail - Error(%s)", _object_name.c_str(), e.what()));
			return false; 
		}
	}	

	

	
			
	// 비동기 접속 


	if(_is_support_ssl == true)
	{
		if(_accept_socket_ssl != nullptr)
		{
			LOG_WRITE(("WARNING : [%s] TcpNetworkManager::PostAccept - _accept_socket_ssl not nullptr", _object_name.c_str()));
			
			//if(_accept_socket_ssl->is_open() == true)
			{
				_accept_socket_ssl->lowest_layer().close();
			}

			delete _accept_socket_ssl; 
			_accept_socket_ssl = nullptr; 
		}
		
		_accept_socket_ssl = new NetSocketSSL(*(_context_pool->GetContext()), *_ssl_context);

		_acceptor->async_accept(_accept_socket_ssl->lowest_layer(), boost::bind(&TcpNetworkManager::OnAcceptSSL, this, boost::asio::placeholders::error));
	}
	else
	{

		if(_accept_socket != nullptr)
		{
			LOG_WRITE(("WARNING : [%s] TcpNetworkManager::PostAccept - _accept_socket not nullptr", _object_name.c_str()));
			
			//if(_accept_socket->is_open() == true)
			{
				_accept_socket->close();
			}

			delete _accept_socket; 
			_accept_socket = nullptr; 
		}
		
		_accept_socket  = new NetTcpSocket(*(_context_pool->GetContext())); 
		_acceptor->async_accept(*_accept_socket, boost::bind(&TcpNetworkManager::OnAccept, this, boost::asio::placeholders::error));
	}

	
	return true; 

}

//====================================================================================================
// Accept 핸들러 
//====================================================================================================
void TcpNetworkManager::OnAccept(const NetErrorCode & error)
{
	uint32_t	ip	= 0; 
	int			port= 0; 
		
	// 에러 
	if(error)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::OnAccept - Error(%d) Message(%s)", _object_name.c_str(), error.value(), error.message().c_str()));
				
		if(_accept_socket != nullptr)
		{
			//if(_accept_socket->is_open() == true)
			{
				_accept_socket->close(); 
			}

			delete _accept_socket; 
			_accept_socket 		= nullptr;
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

	// 연결 콜백 확인 
	if(_network_callback == nullptr)
	{
		if(_accept_socket != nullptr)
		{
			//if(_accept_socket->is_open() == true)
			{
				_accept_socket->close(); 
			}

			delete _accept_socket; 
			_accept_socket = nullptr; 
		}
			
		return; 
	}
	
	// IP/Port 설정 
	try
	{
		ip		= boost::asio::detail::socket_ops::network_to_host_long(_accept_socket->remote_endpoint().address().to_v4().to_ulong()); 
		port	= _accept_socket->remote_endpoint().port();	
	}	
	catch (const std::exception& e)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::OnAccept - Socket Exception - Error(%s)", _object_name.c_str(), e.what()));
		
		if(_accept_socket != nullptr)
		{
			//if(_accept_socket->is_open() == true)
			{
				_accept_socket->close(); 
			}

			delete _accept_socket; 
			_accept_socket = nullptr;
		}

		// Accept 이벤트 설정 
		if(_is_closeing == false)
		{
			PostAccept();
		}
		return;
	}
	
	// 연결 콜백 호출 
	if(_network_callback->OnTcpNetworkAccepted(_object_key, _accept_socket, ip, port) == false)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::OnAccept -  OnTcpNetworkAccepted fail", _object_name.c_str()));
		
		if(_accept_socket != nullptr)
		{
			LOG_WRITE(("ERROR : [%s] TcpNetworkManager::OnAccept - Socket Not nullptr", _object_name.c_str()));
			
			//if(_accept_socket->is_open() == true)
			{
				_accept_socket->close(); 
			}

			delete _accept_socket; 
			_accept_socket = nullptr; 
		}
	}

	_accept_socket = nullptr; 
		
	// Accept 이벤트 설정 
	PostAccept();
	
	return; 
}	


//====================================================================================================
// SSL Accept 핸들러 
//====================================================================================================
void TcpNetworkManager::OnAcceptSSL(const NetErrorCode & error)
{
	// 에러 
	if(error)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::OnSSLAccept - Error(%d) Message(%s)", _object_name.c_str(), error.value(), error.message().c_str()));
				
		if(_accept_socket_ssl != nullptr)
		{
			//if(_accept_socket_ssl->lowest_layer().is_open() == true)
			{
				_accept_socket_ssl->lowest_layer().close(); 
			}

			delete _accept_socket_ssl; 
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

	//auto timer = boost::get_timeout_timer(_accept_socket, _timeout_request);

	_accept_socket_ssl->async_handshake(boost::asio::ssl::stream_base::server, boost::bind(&TcpNetworkManager::OnHandshakeSSL, this, boost::asio::placeholders::error));


	return; 
}


//====================================================================================================
// SSL HandShake 핸들러(accepter)
//====================================================================================================
void TcpNetworkManager::OnHandshakeSSL(const NetErrorCode & error)
{
	uint32_t	ip 		= 0; 
	int 		port	= 0; 
		
	// 에러 
	if(error)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::OnHandshakeSSL - Error(%d) Message(%s)", _object_name.c_str(), error.value(), error.message().c_str()));
				
		if(_accept_socket_ssl != nullptr)
		{
			//if(_accept_socket_ssl->lowest_layer().is_open() == true)
			{
				_accept_socket_ssl->lowest_layer().close(); 
			}

			delete _accept_socket_ssl; 
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

	
	// IP/Port 설정 
	try
	{
		ip 	= boost::asio::detail::socket_ops::network_to_host_long(_accept_socket_ssl->lowest_layer().remote_endpoint().address().to_v4().to_ulong()); 
		port	= _accept_socket_ssl->lowest_layer().remote_endpoint().port();	
	}	
	catch (std::exception& error)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::OnHandshakeSSL - Socket Exception", _object_name.c_str()));
		
		if(_accept_socket_ssl != nullptr)
		{
			//if(_accept_socket_ssl->lowest_layer().is_open() == true)
			{
				_accept_socket_ssl->lowest_layer().close(); 
			}

			delete _accept_socket_ssl; 
			_accept_socket_ssl = nullptr;
		}

		// Accept 이벤트 설정 
		if(_is_closeing == false)
		{
			PostAccept();
		}
		
		return;
	}

	// 연결 콜백 호출 
	if(_network_callback->OnTcpNetworkAcceptedSSL(_object_key, _accept_socket_ssl, ip, port) == false)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::OnHandshakeSSL -  OnTcpNetworkAcceptedSSL return false", _object_name.c_str()));
		
		if(_accept_socket_ssl != nullptr)
		{
			LOG_WRITE(("ERROR : [%s] TcpNetworkManager::OnHandshakeSSL - Socket Not nullptr", _object_name.c_str()));
			
			//if(_accept_socket_ssl->lowest_layer().is_open() == true)
			{
				_accept_socket_ssl->lowest_layer().close(); 
			}

			delete _accept_socket_ssl; 
			_accept_socket_ssl = nullptr; 
		}
	}

	_accept_socket_ssl = nullptr; 
		
	//Accept 이벤트 설정 
	PostAccept();
		
	return;
}



//====================================================================================================
// 검색(IP,Port)
//====================================================================================================
bool TcpNetworkManager::FindIndexKey(uint32_t ip, int port, int & index_key)
{
	bool result = false;
	
	std::lock_guard<std::mutex> network_info_map_lock(_network_info_map_mutex);;

	// 검색 
	for(const auto &item : _network_info_map)
	{
		auto object = item.second;

		if(object->GetRemoteIP() == ip && object->GetRemotePort() == port)
		{
			index_key 	= object->GetIndexKey();
			result		= true; 
			break; 
		}
	}

	return result; 
}

//====================================================================================================
// 연결 여부 (IP,Port)
//====================================================================================================
bool TcpNetworkManager::IsConnected(uint32_t ip, int port)
{
	int 	index_key 	= -1; 
	bool 	result 	= false; 
	
	result = FindIndexKey(ip, port,  index_key);
		
	return (result == true && index_key != -1)? true : false; 
}


//====================================================================================================
// 동기 검색 
//====================================================================================================
std::shared_ptr<TcpNetworkObject> TcpNetworkManager::Find(int index_key, bool erase/* = false*/)
{
	std::shared_ptr<TcpNetworkObject>	object = nullptr;

	std::lock_guard<std::mutex> network_info_map_lock(_network_info_map_mutex);
	
	// 찾기
	auto item = _network_info_map.find(index_key);
	if( item != _network_info_map.end() )
	{
		object = item->second;

		if (erase)
			_network_info_map.erase(item);
	}


	return object;
}

//====================================================================================================
// 삭제
//====================================================================================================
bool TcpNetworkManager::Remove(int index_key) 
{
	bool result = false; 
		
	auto object = Find(index_key, true);

	if (object != nullptr)
	{
		//종료 요청 
		object->PostClose();
		result = true; 
	}

	return result; 
}

//====================================================================================================
// 삭제(목록) 
//====================================================================================================
void TcpNetworkManager::Remove(std::vector<int> IndexKeys) 
{
	std::shared_ptr<TcpNetworkObject>	object;
	
	for(auto index_key : IndexKeys)
	{
		auto object = Find(index_key, true);

		if (object != nullptr)
		{
			//종료 요청 
			object->PostClose();
		}
	}

	return; 
}

//====================================================================================================
// 전체 삭제   
// - 연결 종료 
//====================================================================================================
void TcpNetworkManager::RemoveAll() 
{
	std::shared_ptr<TcpNetworkObject>	object;

	std::lock_guard<std::mutex> network_info_map_lock(_network_info_map_mutex);;

	// 삭제
	for(auto item = _network_info_map.begin() ; item != _network_info_map.end(); )
	{
		object = item->second;

		//종료 요청 
		object->PostClose(); 

		//삭제 
		_network_info_map.erase(item++);
	}

	_network_info_map.clear(); 

	return;
}

//====================================================================================================
// 연결 개수 
//====================================================================================================
uint32_t TcpNetworkManager::GetCount()
{
	uint32_t nCount = 0; 

	std::lock_guard<std::mutex> network_info_map_lock(_network_info_map_mutex);;

	nCount = (uint32_t)_network_info_map.size();

	return nCount;	
}

//====================================================================================================
// 네트워크 전체 트래픽  
// - bps 
//====================================================================================================
uint64_t TcpNetworkManager::GetTotalTrafficRate()
{
	uint64_t traffic_rate = 0;
	double send_bitrate;
	double recv_bitrate;

	std::lock_guard<std::mutex> network_info_map_lock(_network_info_map_mutex);;

	for (const auto &item : _network_info_map)
	{
		auto object = item.second;

		if (object->GetTrafficRate(send_bitrate, recv_bitrate))
		{
			traffic_rate += (send_bitrate + recv_bitrate);
		}
	}

	return traffic_rate;

}


//====================================================================================================
// 연결 (time out)
// - timeout( 0 - 무한) : 밀리초  단위 
//====================================================================================================
bool TcpNetworkManager::TimeoutConnect(const char * host,int port, int timeout, SOCKET & socket_handle)
{
#ifdef WIN32
	SOCKADDR_IN address;
	int 		nResult;
	ULONG 		nNonBlocking; 
	TIMEVAL 	timevalue;
	fd_set 		fdset;
	
	address.sin_addr.s_addr = inet_addr(host);
	address.sin_family = AF_INET;
	address.sin_port = htons((short)port);
	
	timevalue.tv_sec = timeout/1000;
	timevalue.tv_usec = 0;

	socket_handle = INVALID_SOCKET;
	socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Set non-block
	nNonBlocking = true;
	if(ioctlsocket(socket_handle, FIONBIO, &nNonBlocking ) == SOCKET_ERROR)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::TimeoutConnect - option set fail", _object_name.c_str()));
		return false;
	}

	// Connect
	nResult = connect(socket_handle, (LPSOCKADDR)&address, sizeof(address));
	
		
	// 연결 성공 
	if(nResult == 0)
	{
		// Reset non-block
		nNonBlocking = false;
		ioctlsocket(socket_handle, FIONBIO, &nNonBlocking);
		
		return true;
	}

	if(nResult == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::TimeoutConnect - connect fail", _object_name.c_str()));
		return false;
	}

	// Reset non-block
	nNonBlocking = false;
	ioctlsocket(socket_handle, FIONBIO, &nNonBlocking);
	
 
	FD_ZERO( &fdset );
	FD_SET( socket_handle, &fdset );

	// Select
	if(select(0, nullptr, &fdset, nullptr, &timevalue ) == SOCKET_ERROR)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::TimeoutConnect - select time over", _object_name.c_str()));
		closesocket(socket_handle);
		socket_handle = INVALID_SOCKET;
		return false;
	}

	if(!FD_ISSET(socket_handle, &fdset))
	{		
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::TimeoutConnect - FD_ISSET fail", _object_name.c_str()));
		closesocket(socket_handle);
		socket_handle = INVALID_SOCKET;
		return false;

	}

	return true;
#else

	SOCKADDR_IN 	address;
	int				nFlags;
	int 			nResult;
	int				nError 			= 0;
	socklen_t 		nErrorLength 	= sizeof(nError);
	
	address.sin_addr.s_addr = inet_addr(host);
	address.sin_family = AF_INET;
	address.sin_port = htons((short)port);

	socket_handle = INVALID_SOCKET;
	socket_handle =  socket(AF_INET, SOCK_STREAM, 0);

	// Set no-block
	nFlags = fcntl(socket_handle, F_GETFL, 0);
	fcntl(socket_handle, F_SETFL, nFlags | O_NONBLOCK);

	// Connect
	nResult = connect(socket_handle, (struct sockaddr *)&address, sizeof(address)); 

	// 연결 성공 
	if(nResult == 0)
	{
		if(fcntl(socket_handle, F_SETFL, nFlags) == -1)
		{
			if(socket_handle != INVALID_SOCKET)
			{
				close(socket_handle);
				socket_handle = INVALID_SOCKET;
			}
			return false;
		}
		
		return true;
	}

	// 실패 
	if(errno != EINPROGRESS )
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::TimeoutConnect - connect fail", _object_name.c_str()));

		if(socket_handle != INVALID_SOCKET)
		{
			close(socket_handle);
			socket_handle = INVALID_SOCKET;
		}
		return false;
	}


	struct epoll_event 	connectionEvent;
    int 				hEpoll				= -1;
    struct epoll_event 	processableEvents;
    uint32_t 		nEventCount			= -1;

	// epoll 생성 
	hEpoll = epoll_create(1);
    if(hEpoll == -1)
    {
    	LOG_WRITE(("ERROR : [%s] TcpNetworkManager::TimeoutConnect - epoll_create fail - Error(%d)", _object_name.c_str(), errno));
		close(socket_handle);
		socket_handle = INVALID_SOCKET;
		return false;
    }     
	
	//  추가 
	
	memset(&connectionEvent, 0, sizeof(connectionEvent));
    connectionEvent.data.fd = socket_handle;
    connectionEvent.events 	= EPOLLOUT | EPOLLIN | EPOLLERR;

	nResult = epoll_ctl(hEpoll, EPOLL_CTL_ADD, socket_handle, &connectionEvent);
    if(nResult != 0)
    {
       	LOG_WRITE(("ERROR : [%s] TcpNetworkManager::TimeoutConnect - epoll_ctl(add) fail - Error(%d)", _object_name.c_str(), errno));
		close(socket_handle);
		close(hEpoll);
		socket_handle = INVALID_SOCKET;
		return false;
    }

	// 무한 대기 설정 
	if(timeout <= 0) timeout = -1; 
				
    nEventCount = epoll_wait(hEpoll, &processableEvents, 1, timeout);

	// 이벤트 확인(시간 초과) 
    if(nEventCount < 0)
    {
       	LOG_WRITE(("ERROR : [%s] TcpNetworkManager::TimeoutConnect - epoll_wait time over", _object_name.c_str()));
		close(socket_handle);
		close(hEpoll);
		socket_handle = INVALID_SOCKET;
		return false;
    }

	close(hEpoll);
	
	// 에러 확인 
    if(getsockopt(socket_handle, SOL_SOCKET, SO_ERROR, (void *)&nError, &nErrorLength) != 0)
    {
       	LOG_WRITE(("ERROR : [%s] TcpNetworkManager::TimeoutConnect - getsockopt fail", _object_name.c_str()));
		close(socket_handle);
		socket_handle = INVALID_SOCKET;
		return false;
    }

    if(nError != 0) 
    {
      	LOG_WRITE(("ERROR : [%s] TcpNetworkManager::TimeoutConnect - socket error - error(%d)", _object_name.c_str(), nError));
		close(socket_handle);
		socket_handle = INVALID_SOCKET;
		return false;
    }   

	// 소켓 속성 reset
	if(fcntl(socket_handle, F_SETFL, nFlags) == -1)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::TimeoutConnect - fcntl fail", _object_name.c_str()));
		close(socket_handle);
		socket_handle = INVALID_SOCKET;
		return false;
	}

/*
	socklen_t length = 0;
	if(getpeername(socket_handle, (struct sockaddr *)&address, &length) != 0)
	{
		LOG_WRITE(("ERROR : [%s] TcpNetworkManager::TimeoutConnect - getpeername fail", _object_name.c_str()));
		
		close(socket_handle);
		socket_handle = INVALID_SOCKET;
		return false;
	}
*/
	return true;
#endif

}

//====================================================================================================
// 로컬 IP 획득 
//====================================================================================================
std::string TcpNetworkManager::GetLocalIP()
{
	std::string 			local_ip;
	boost::asio::io_context ioservice; 

	boost::asio::ip::tcp::resolver			resolver(ioservice); 
	boost::asio::ip::tcp::resolver::query	query(boost::asio::ip::host_name(), ""); 
	
	auto it = resolver.resolve(query); 
		
	while(it != boost::asio::ip::tcp::resolver::iterator())
	{
		boost::asio::ip::address addr = (it++)->endpoint().address();
		if(addr.is_v4())
		{
			local_ip = addr.to_string();
			break; 			
		}
	}

	return local_ip;
}
