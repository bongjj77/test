//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "tcp_network_manager.h"
#include "udp_network_object.h" 
#include "network_context_pool.h"

typedef std::map<int, std::shared_ptr<UdpNetworkObject>> 	UdpNetworkInfoMap; //Key : 자체 구분 IndexKey 

#define LOCK_NETWORK_INFO  					std::lock_guard<std::mutex> network_info_map_lock(_network_info_map_mutex);

#define DEFAULT_NETWORK_TIMEOUT				(3000) 
#define DEFAULT_NETWORK_RECONNECT_COUNT		(3) 

//====================================================================================================
// NetworkManager
//====================================================================================================
class UdpNetworkManager : public NetworkManager
{
public :
	UdpNetworkManager(int object_key);
	virtual ~UdpNetworkManager();

public : 
	virtual bool		Create(INetworkCallback * callback, std::shared_ptr<NetworkContextPool> service_pool, int listen_port, std::string object_name);
	virtual bool		Create(INetworkCallback * callback, std::shared_ptr<NetworkContextPool> service_pool, std::string object_name) { return Create(callback, service_pool, 0, object_name); }
	
	void				Close(int index_key){ Remove(index_key); } 
	bool				FindIndexKey(uint32_t ip, int port, int & index_key); 
	bool				IsConnected(uint32_t ip, int port);
	virtual void		PostRelease() {};
	virtual bool		Remove(int index_key);
	virtual void		Remove(std::vector<int> IndexKeys); 
	virtual void		RemoveAll();
	virtual uint32_t	GetCount();
	
	bool				IsServerModule(){ return (_listen_port != 0); }
	bool				IsClientModule(){ return (_listen_port == 0); }
	int					GetCurrentIndexKey(){ return _index_key; } 
	
	uint64_t			GetTotalTrafficRate();
	
protected :
	void				Release();
		
	virtual int			Insert(std::shared_ptr<UdpNetworkObject> object, bool bKeepAliveCheck = false, uint32_t keepalive_check_time = 0);
	std::shared_ptr<UdpNetworkObject> Find(int index_key, bool erase = false);

	
protected : 	
	std::shared_ptr<NetworkContextPool> _context_pool;
	int					_index_key; 
	uint32_t			_max_createing_count; 
	UdpNetworkInfoMap	_network_info_map;
	std::mutex			_network_info_map_mutex;
	int					_listen_port; 
	INetworkCallback	*_network_callback; 	
}; 
