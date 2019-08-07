//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once 

#include "common_header.h"
#include <mutex>
#include <map>
#include <thread>

//====================================================================================================
// ThreadTimer Interface
//====================================================================================================
class ITimerCallback
{
public:
	virtual void OnThreadTimer(uint32_t timer_id, bool &delete_timer) = 0;
};

struct TimerInfo
{
	uint32_t elapse;		// 몇ms마다 타이머 이벤트 발생할것인지
	uint64_t recent_bell;	// 가장 최근에 이벤트 발생한 시간
};

//====================================================================================================
// ThreadTimer
//====================================================================================================
class ThreadTimer
{

public:
	ThreadTimer();
	virtual ~ThreadTimer( );

public:
	bool Create(ITimerCallback *callback, uint32_t interval=100); // 단위ms
	void Destroy( );

	bool SetTimer(uint32_t timer_id, uint32_t Elapse); // 단위는 ms, 이미 있으면 Elapse만 변경
	bool KillTimer(uint32_t timer_id);

private:

	void TimerProcess();


private:
	ITimerCallback *_timer_interface;
	uint32_t _interval;

	std::map<uint32_t, TimerInfo*> _timer_map;
	std::recursive_mutex _timer_map_mutex; // 동일 Thread에서 접속 가능

	bool _is_timer_thread_stop;
	std::thread _timer_thread;
};


