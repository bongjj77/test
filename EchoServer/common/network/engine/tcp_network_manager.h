#pragma once
#include "network_manager.h" 
#include "tcp_network_object.h" 
#include "network_context_pool.h"

typedef std::map<int, std::shared_ptr<TcpNetworkObject>> 	TcpNetworkInfoMap; //Key : IndexKey 

#define LOCK_NETWORK_INFO std::lock_guard<std::mutex> network_info_map_lock(_network_info_map_mutex);

#define DEFAULT_NETWORK_TIMEOUT (3000) 
#define DEFAULT_NETWORK_RECONNECT_COUNT (3) 

//====================================================================================================
// NetworkManager
//====================================================================================================
//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

class TcpNetworkManager : public NetworkManager
{
public :
	TcpNetworkManager(int object_key);
	virtual ~TcpNetworkManager();

public : 
	virtual bool Create(INetworkCallback * callback, 
						std::shared_ptr<NetworkContextPool>service_pool, 
						int listen_port, 
						std::string object_name, 
						bool private_accepter_service = false);

	virtual bool Create(INetworkCallback * callback, 
						std::shared_ptr<NetworkContextPool> service_pool, 
						std::string object_name)
	{ 
		return Create(callback, service_pool, 0, object_name); 
	}

	virtual bool CreateSSL(INetworkCallback		*callback,
						std::shared_ptr<NetworkContextPool> service_pool,
						std::string 			cert_file,
						std::string				key_file,
						std::string				verify_file,	
						int 					listen_port, 
						std::string 			object_name, 
						bool 					private_accepter_service = false,
						int						timeout_request			= 5, 
						long 					timeout_content			= 300);

	virtual bool CreateSSL(INetworkCallback * callback,
							std::shared_ptr<NetworkContextPool> service_pool,
							std::string object_name)
	{
		std::string cert_file;
		std::string key_file;
		std::string verify_file;

		return CreateSSL(callback, service_pool, cert_file, key_file, verify_file, 0, object_name);
	}

	bool				PostConnect(uint32_t ip, int port, std::shared_ptr<std::vector<uint8_t>> connected_param);
	bool				PostConnect(std::string & ip_string, int port, std::shared_ptr<std::vector<uint8_t>> connected_param);
	bool				PostConnectSSL(uint32_t ip, std::string host, int port, std::shared_ptr < std::vector<uint8_t>> connected_param);

	bool				VerifyCallback(bool preverified, boost::asio::ssl::verify_context& ctx);

	
	void				Close(int index_key){ Remove(index_key); } 
	bool				FindIndexKey(uint32_t ip, int port, int & index_key); 
	bool				IsConnected(uint32_t ip, int port);
	
 	void				PostRelease();
	virtual bool		Remove(int index_key);
	virtual void		Remove(std::vector<int> IndexKeys); 
	virtual void		RemoveAll();
	virtual uint32_t	GetCount();
	
	bool				IsServerModule(){ return (_listen_port != 0); }
	bool				IsClientModule(){ return (_listen_port == 0); }
	int					GetCurrentIndexKey(){ return _index_key; } 
	
	uint64_t			GetTotalTrafficRate();

	static std::string	GetLocalIP();
	
protected :
	void				Release();
		
	virtual int			Insert(std::shared_ptr<TcpNetworkObject> object, 
								bool bKeepAliveCheck = false, 
								uint32_t keepalive_check_time = 0);

	std::shared_ptr<TcpNetworkObject> Find(int index_key, bool erase = false);

	bool PostAccept(); 

	void OnAccept(const NetErrorCode & error);

	virtual void OnConnected(const NetErrorCode & error, 
							NetTcpSocket * socket, 
							std::shared_ptr<std::vector<uint8_t>> connected_param, 
							uint32_t ip, 
							int port);
	
	virtual void OnConnectedSSL(const NetErrorCode & error,
							NetSocketSSL * socket,
							std::shared_ptr<std::vector<uint8_t>> connected_param,
							uint32_t ip,
							int port);

	void OnAcceptSSL(const NetErrorCode & error);

	void OnHandshakeSSL(const NetErrorCode & error);
	void OnHandshakeSSL(const NetErrorCode & error, NetSocketSSL * socket, std::shared_ptr<std::vector<uint8_t>> connected_param, uint32_t ip, int port);

private : 
	bool TimeoutConnect(const char * host,int port, int timeout, SOCKET & socket_handle);
	
protected : 
	
	bool				_is_private_accepter_service;
		
	int					_index_key; 
	uint32_t			_max_createing_count; 
	TcpNetworkInfoMap	_network_info_map;
	std::mutex			_network_info_map_mutex;

	int					_listen_port; 
	NetAcceptor			*_acceptor; 
	NetTcpSocket		*_accept_socket;
	NetTimer			*_release_timer;
	
	INetworkCallback	*_network_callback;
	bool				_is_closeing;

	bool				_is_support_ssl;
	std::shared_ptr<boost::asio::ssl::context> _ssl_context;
	int					_timeout_request;
	int					_timeout_content;
	NetSocketSSL		*_accept_socket_ssl;	
}; 
