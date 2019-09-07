//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "tcp_network_manager.h" 
#include "tcp_tls_network_object.h"

//====================================================================================================
// NetworkManager
//====================================================================================================
class TcpTlsNetworkManager : public TcpNetworkManager
{
public :
	TcpTlsNetworkManager(int object_key);
	virtual ~TcpTlsNetworkManager();

public : 
  
	virtual bool CreateSSL(std::shared_ptr<ITlsTcpNetwork> callback,
						std::shared_ptr<NetworkContextPool> service_pool,
						std::string 			cert_file,
						std::string				key_file,
						std::string				verify_file,	
						int 					listen_port, 
						std::string 			object_name, 
						bool 					private_accepter_service = false);

	virtual bool CreateSSL(std::shared_ptr<ITlsTcpNetwork> callback,
							std::shared_ptr<NetworkContextPool> service_pool,
							std::string object_name)
	{
		std::string cert_file;
		std::string key_file;
		std::string verify_file;

		return CreateSSL(callback, service_pool, cert_file, key_file, verify_file, 0, object_name);
	}
	 
	virtual bool PostConnect(uint32_t ip, std::string host, int port, std::shared_ptr < std::vector<uint8_t>> connected_param);

	bool VerifyCallback(bool preverified, boost::asio::ssl::verify_context& ctx); 
	
protected :
	virtual void Release();

	virtual void OnConnectedSSL(const NetErrorCode & error,
								std::shared_ptr<NetTlsSocket> socket,
								std::shared_ptr<std::vector<uint8_t>> connected_param,
								uint32_t ip,
								int port);

	virtual bool PostAccept();

	virtual void OnAccept(const NetErrorCode & error);

	void OnHandshakeSSL(const NetErrorCode & error);

	void OnHandshakeSSL(const NetErrorCode & error, 
						std::shared_ptr<NetTlsSocket> socket, 
						std::shared_ptr<std::vector<uint8_t>> connected_param, 
						uint32_t ip, 
						int port); 
	
protected : 
 	std::shared_ptr<boost::asio::ssl::context> _ssl_context;
	std::shared_ptr<NetTlsSocket> _accept_socket_ssl;	
}; 
