//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "network_object_header.h"
#include "../../common_header.h"
#include <thread>

//====================================================================================================
// NetworkContextPool 
//====================================================================================================
class NetworkContextPool
{
public :
	NetworkContextPool(int pool_count);
	virtual ~NetworkContextPool();

public : 
	void Run();
	void Stop();
	std::shared_ptr<NetIoContext> GetContext();
	std::shared_ptr<NetIoContext> GetPrivateContext();

	bool IsRun(){ return _is_run; }
	 
private :
	std::vector<std::shared_ptr<NetIoContext>> _io_context_list;
	std::vector<std::shared_ptr<boost::asio::io_context::work>> _work_list;
	std::vector<std::shared_ptr<std::thread>>	_network_thread_list;

	int _pool_count;
	int _private_io_context_index; 
	int _io_context_index; 
	std::mutex _io_context_index_mutex;
	bool _is_run; 
};
