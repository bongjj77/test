#include "stdafx.h"
#include "thread_timer.h"

#define _MIN_(X,Y) ((X) < (Y) ? (X) : (Y))   
#define _MAX_(X,Y) ((X) > (Y) ? (X) : (Y))  

#ifdef WIN32
#include <process.h>
#else
#include "pthread.h"
#endif


ThreadTimer::ThreadTimer()
{
	// 초기화
	_timer_interface		= nullptr;
	_stop_thread	= false;
	_thread		= 0;

	// 맵관련 초기화
	_timer_map.clear();
}

ThreadTimer::~ThreadTimer()
{
	// 삭제
	Destroy();

	// 맵관련 해제
	_timer_map.clear();
}

bool ThreadTimer::Create(ITimerCallback *callback, uint32_t Resolution)
{
	// 파라미터 체크
	if( !callback ) 
	{ 
		return false; 
	}

	// 해상도 범위 정하기
	Resolution = _MAX_(Resolution, (uint32_t)1);

	// 저장
	_timer_interface = callback;
	_resolution	= Resolution;

	// 맵삭제
	_timer_map.clear();

	// 타이머 스레드 생성
	_stop_thread = false;
#ifdef WIN32
	_thread = (HANDLE)_beginthreadex(nullptr, 0, TimerThread, (void*)this, 0, nullptr);
#else
	pthread_create(&_thread, nullptr, TimerThread, this);
#endif
	if( !_thread ) { goto $error_occurred; }

	return true;

	//
$error_occurred:
	Destroy();
	return false;
}

void ThreadTimer::Destroy( )
{
	
	// 스레드 중지
	if( _thread )
	{
		_stop_thread = true;
#ifdef WIN32
		WaitForSingleObject(_thread, INFINITE);
		CloseHandle(_thread); _thread = nullptr;
#else
		pthread_join(_thread, nullptr);
		_thread = 0;
#endif
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

bool ThreadTimer::SetTimer(uint32_t timer_id, uint32_t Elapse) // 단위는 ms
{
	
	std::lock_guard<std::recursive_mutex> timer_map_lock(_timer_map_mutex);

	// 이미 있는지 체크
	auto item = _timer_map.find(timer_id);

	// 있으면 속성을 변경
	if( item != _timer_map.end() )
	{
		item->second->elapse		= Elapse;
		item->second->recent_bell	= GetTickCount();
	}
	// 없으면 새로 추가
	else
	{
		TIMER_INFO *pt_timer = nullptr;

		// 타이머 생성
		pt_timer = new TIMER_INFO;
		pt_timer->elapse		= Elapse;
		pt_timer->recent_bell	= GetTickCount();

		// 추가
		_timer_map.insert(std::pair<uint32_t, TIMER_INFO *>(timer_id, pt_timer));
	}

	return true;
}

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


uint32_t ThreadTimer::TimerProcess( )
{
	while(!_stop_thread)
	{
		// 쉬기
		Sleep(_resolution);

		// 타이머 처리
		std::unique_lock<std::recursive_mutex> timer_map_lock(_timer_map_mutex);
		{
			DWORD			cur_time;

			// 현재 시간 구하기
			cur_time = GetTickCount();
			 
			// 순환
			for(auto item=_timer_map.begin(); item!=_timer_map.end(); )
			{
				uint32_t			timer_id;
				TIMER_INFO *pt_timer = nullptr;

				// 타이머 정보 얻기
				timer_id = item->first;
				pt_timer = item->second;

				// 콜백을 호출하기전에 이 다음 아이템 지정
				// 이렇게 해야 콜백중에 KillTimer()를 호출하여 해쉬아이템을 삭제하더라도 문제없음
				++item;

				// 타이머를 호출할 시간이 됐으면 호출!
				if(cur_time > pt_timer->recent_bell &&  cur_time - pt_timer->recent_bell >= pt_timer->elapse )
				{
					// 시간을 먼저 기록후 콜백을 호출한다.
					// 콜백호출중에 KillTimer()로 삭제하면 memory access violation이 발생하기 때문이다.
					pt_timer->recent_bell = cur_time;

					bool delete_timer = false; 
					_timer_interface->OnThreadTimer(timer_id, delete_timer);
					
					

					if (delete_timer)
					{
						KillTimer(timer_id);
					}
				}
			}
		}
		timer_map_lock.unlock();
	}

	return 0;
}

#ifdef WIN32
uint32_t __stdcall ThreadTimer::TimerThread(LPVOID pParameter)
{
	return ((ThreadTimer*)pParameter)->TimerProcess();
}
#else
void* ThreadTimer::TimerThread(void* pParameter)
{

	((ThreadTimer*)pParameter)->TimerProcess();

	return 0;
}
#endif
