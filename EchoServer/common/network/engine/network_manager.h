#pragma once
#include "tcp_network_object.h" 
#include "network_context_pool.h"

//====================================================================================================
// NetworkManager
//====================================================================================================
class NetworkManager
{
public :
	NetworkManager(int object_key);
	virtual ~NetworkManager();

	virtual void PostRelease() = 0;
	virtual bool Remove(int index_key) = 0;
	virtual void Remove(std::vector<int> IndexKeys) = 0;
	virtual void RemoveAll() = 0;
	virtual uint32_t GetCount() = 0;

	int GetObjectKey() { return _object_key; }

protected: 
	int _object_key;
	std::string _object_name;
	std::shared_ptr<NetworkContextPool> _context_pool;
}; 
