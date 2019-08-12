//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "thread_timer.h"

//====================================================================================================
// Constructor  
//====================================================================================================
ThreadTimer::ThreadTimer()
{
	_timer_interface = nullptr;
	_is_timer_thread_stop = false;
}

//====================================================================================================
// Destructor 
//====================================================================================================
ThreadTimer::~ThreadTimer()
{
	Destroy();
}

//====================================================================================================
// Destructor 
//====================================================================================================
bool ThreadTimer::Create(ITimerCallback *callback, uint32_t interval)
{
 	if(callback == nullptr) 
	{ 
		return false; 
	}
 
	_timer_interface = callback;
	_interval = interval;
 
	_timer_map.clear();

	 
	_is_timer_thread_stop = false;
	_timer_thread = std::thread(&ThreadTimer::TimerProcess, this);

	return true;
}

//====================================================================================================
// Destroy 
//====================================================================================================
void ThreadTimer::Destroy( )
{ 
	if (_is_timer_thread_stop == false)
	{
		_is_timer_thread_stop = true;
		_timer_thread.join();
	}

	std::lock_guard<std::recursive_mutex> timer_map_lock(_timer_map_mutex);

	// 맵 삭제
	_timer_map.clear();

	// 기타 초기화
	_timer_interface = nullptr;
}

//====================================================================================================
// SetTimer 
//====================================================================================================
bool ThreadTimer::SetTimer(uint32_t timer_id, uint32_t Elapse) // 단위는 ms
{
	
	std::lock_guard<std::recursive_mutex> timer_map_lock(_timer_map_mutex);

	auto item = _timer_map.find(timer_id);

	if( item != _timer_map.end() )
	{
		item->second->elapse		= Elapse;
		item->second->recent_bell	= get_tick_count64();
	}
	else
	{
		auto timer_info = std::make_shared<TimerInfo>();
		timer_info->elapse		= Elapse;
		timer_info->recent_bell	= get_tick_count64();
	
		_timer_map[timer_id] = timer_info;
	}

	return true;
}

//====================================================================================================
// KillTimer 
//====================================================================================================
bool ThreadTimer::KillTimer(uint32_t timer_id)
{
 
	std::lock_guard<std::recursive_mutex> timer_map_lock(_timer_map_mutex);

	auto item = _timer_map.find(timer_id);

	if( item != _timer_map.end() )
	{
	 	_timer_map.erase(item);
	}

	return true;
}

//====================================================================================================
// TimerProcess 
//====================================================================================================
void ThreadTimer::TimerProcess( )
{
	while(!_is_timer_thread_stop)
	{
		SleepWait(_interval);

		std::unique_lock<std::recursive_mutex> timer_map_lock(_timer_map_mutex);
		uint64_t current_tick = get_tick_count64();
			 
	 	for(auto item=_timer_map.begin(); item!=_timer_map.end(); )
		{
			uint32_t timer_id = item->first;
			auto timer = item->second;

			++item;

			
			if(current_tick > timer->recent_bell && current_tick - timer->recent_bell >= timer->elapse )
			{
				timer->recent_bell = current_tick;

				bool delete_timer = false; 
				_timer_interface->OnThreadTimer(timer_id, delete_timer);
				
				if (delete_timer)
				{
					KillTimer(timer_id);
				}
			}
		}
		 
		timer_map_lock.unlock();
	}
}