#pragma once 

#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
	#define NOMINMAX
	#include <windows.h>
	#include <winsock2.h>
#else
	#include <stdarg.h>
#endif

#include <mutex>
#include <map>

#ifdef WIN32
	typedef void* THREAD_HANDLE;	 
#else
	typedef pthread_t THREAD_HANDLE;
#endif

//====================================================================================================
// ThreadTimer Interface
//====================================================================================================
class ITimerCallback
{
public:
	virtual void OnThreadTimer(uint32_t timer_id, bool &delete_timer) = 0;
};

//====================================================================================================
// ThreadTimer
//====================================================================================================
class ThreadTimer
{
public:

	struct TIMER_INFO // new, delete사용
	{
		uint32_t elapse;		// 몇ms마다 타이머 이벤트 발생할것인지
		uint32_t recent_bell;	// 가장 최근에 이벤트 발생한 시간
	};
  

public:
	ThreadTimer();
	virtual ~ThreadTimer( );

public:
	bool Create(ITimerCallback *callback, uint32_t Resolution=100); // 단위ms
	void Destroy( );

	bool SetTimer(uint32_t timer_id, uint32_t Elapse); // 단위는 ms, 이미 있으면 Elapse만 변경
	bool KillTimer(uint32_t timer_id);

private:

#ifdef WIN32
	static uint32_t __stdcall 	TimerThread(void *pParameter);
#else
	static void * TimerThread(void* pParameter);
#endif
	uint32_t TimerProcess( );

private:
	ITimerCallback * _timer_interface;
	uint32_t _resolution;

	std::map<uint32_t, TIMER_INFO*> _timer_map;
	std::recursive_mutex _timer_map_mutex; // 동일 Thread에서 접속 가능

	bool _stop_thread;
	THREAD_HANDLE _thread;
};


