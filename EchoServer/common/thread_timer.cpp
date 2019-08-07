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
	// 초기화
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
	for(auto item=_timer_map.begin(); item!=_timer_map.end(); ++item)
	{
		delete item->second; 
		item->second = nullptr;
	}
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

	// 이미 있는지 체크
	auto item = _timer_map.find(timer_id);

	// 있으면 속성을 변경
	if( item != _timer_map.end() )
	{
		item->second->elapse		= Elapse;
		item->second->recent_bell	= get_tick_count64();
	}
	// 없으면 새로 추가
	else
	{
		TimerInfo *timer = nullptr;

		// 타이머 생성
		timer = new TimerInfo;
		timer->elapse		= Elapse;
		timer->recent_bell	= get_tick_count64();

		// 추가
		_timer_map[timer_id] = timer;
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
		// 있으면 삭제
		delete item->second; 
		item->second = nullptr;
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
		Sleep(_interval);
 
		std::unique_lock<std::recursive_mutex> timer_map_lock(_timer_map_mutex);
		uint64_t current_tick = get_tick_count64();
			 
	 	for(auto item=_timer_map.begin(); item!=_timer_map.end(); )
		{
			uint32_t timer_id;
			TimerInfo *timer = nullptr;

			timer_id = item->first;
			timer = item->second;

			// 콜백을 호출하기전에 이 다음 아이템 지정
			// 이렇게 해야 콜백중에 KillTimer()를 호출하여 해쉬아이템을 삭제하더라도 문제없음
			++item;

			// 타이머를 호출할 시간이 됐으면 호출!
			if(current_tick > timer->recent_bell && current_tick - timer->recent_bell >= timer->elapse )
			{
				// 시간을 먼저 기록후 콜백을 호출한다.
				// 콜백호출중에 KillTimer()로 삭제하면 memory access violation이 발생하기 때문이다.
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