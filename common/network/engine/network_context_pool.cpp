//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "../../common_header.h"
#include "network_context_pool.h" 
#include <stdexcept>

//====================================================================================================
// Constructor  
//====================================================================================================
NetworkContextPool::NetworkContextPool(int pool_count)
{
	if(pool_count <= 0) _pool_count = std::thread::hardware_concurrency();
	else _pool_count = pool_count; 
	
	LOG_WRITE(("Network pool count %d", _pool_count ));
		
	_is_run = false; 
	_io_context_index = 0;
	_private_io_context_index = 0;

	for(int index = 0 ; index < _pool_count ; index++)
	{
		auto io_context = std::make_shared<NetIoContext>();
		auto work = std::make_shared<boost::asio::io_context::work>(boost::asio::io_context::work(*io_context));
		
		_io_context_list.push_back(io_context);
		_work_list.push_back(work);
	}

}

//====================================================================================================
// Destructor 
//====================================================================================================
NetworkContextPool::~NetworkContextPool()
{
	Stop();	
}

//====================================================================================================
// Run 
//====================================================================================================
void NetworkContextPool::Run()
{
	if(_is_run == true) 
	{
		return; 
	}
			   	 
	for(uint32_t index = 0 ; index < _pool_count ; ++index)
	{
		auto thread = std::make_shared<std::thread>(boost::bind(&NetIoContext::run, _io_context_list[index]));
		_network_thread_list.push_back(thread);

#ifdef WIN32
		bool res = SetThreadAffinityMask(thread->native_handle(), 1u << index);
		assert(res);
#else
		
#endif 
	}

	_is_run = true; 
}

//====================================================================================================
// Stop 
//====================================================================================================
void NetworkContextPool::Stop()
{
	if(_is_run == false) 
	{
		return; 
	}

	// context stop
	for (int index = 0 ; index < _pool_count ; ++index)
	{
		_io_context_list[index]->stop();
	}
	
	// thread wait
	for (int index = 0 ; index < _pool_count ; ++index)
	{
		_network_thread_list[index]->join();
	}
	
	_is_run = false; 
}

//====================================================================================================
// io context 
//====================================================================================================
std::shared_ptr<NetIoContext> NetworkContextPool::GetContext()
{
	std::shared_ptr<NetIoContext> io_context;

	if(_is_run == false) 
	{	
		return std::shared_ptr<NetIoContext>();
	}
	
	std::lock_guard<std::mutex> service_index_lock(_io_context_index_mutex);;
		
	io_context = _io_context_list[_io_context_index++];
	

	if(_io_context_index >= _pool_count)
	{
		//_service_index = 0; 		
		_io_context_index = _private_io_context_index;

	}
	
	return io_context;
}

//====================================================================================================
// private io context
// max : pool count 30%
//====================================================================================================
std::shared_ptr<NetIoContext> NetworkContextPool::GetPrivateContext()
{
	int index = 0; 
	std::shared_ptr<NetIoContext> io_context;
	if(_is_run == false) 
	{
		return std::shared_ptr<NetIoContext>();
	}
	
	std::lock_guard<std::mutex> service_index_lock(_io_context_index_mutex);;

	// acceptor private io context(max 30%)
	if(_private_io_context_index < _pool_count/3)
	{
		index = _private_io_context_index++; 
	}
	else
	{
		index = _io_context_index++;

		if(_io_context_index >= _pool_count)
		{
			_io_context_index = _private_io_context_index;
		}
	}
	
	io_context = _io_context_list[index];
	
	return io_context;
}
