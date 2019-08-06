#pragma once

#ifndef WIN32

//====================================================================================================
// INCLUDE
//====================================================================================================
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <ext/hash_map>

#pragma pack(push, 1)


//====================================================================================================
// 리눅스를 위한 타입설정
//====================================================================================================
typedef void						VOID, *PVOID, *LPVOID, *LPCVOID;
typedef char						*PSTR;
typedef char						CHAR, *PCHAR;
typedef unsigned char				BYTE, *PBYTE, UCHAR, *PUCHAR;
typedef short						SHORT, *PSHORT;
typedef unsigned short				USHORT, *PUSHORT;
typedef int							LONG, *PLONG;
typedef unsigned int				uint32_t, *PUINT, ULONG, *PULONG, DWORD, *PDWORD;

typedef signed char					INT8, *PINT8;
typedef signed short				INT16, *PINT16;
typedef signed int					INT32, *PINT32;
typedef int64_t						INT64, *PINT64;
typedef unsigned char				UINT8, *PUINT8;
typedef unsigned short				UINT16, *PUINT16;
typedef unsigned int				UINT32, *PUINT32;
typedef uint64_t					UINT64, *PUINT64;

typedef int64_t						LONGLONG;
typedef uint64_t					ULONGLONG;

// 구조체 변수 정의
typedef struct _LARGE_INTEGER
{
	LONGLONG	QuadPart;
} LARGE_INTEGER;

typedef struct _ULARGE_INTEGER
{
	ULONGLONG	QuadPart;
} ULARGE_INTEGER;

// 기타정의
#define WINAPI
#ifndef _SOCKET_TYPE_
#undef SOCKET
typedef int     SOCKET;
#define _SOCKET_TYPE_
#endif
#define SOCKET						int
//#define HANDLE						int		//32bit
#define HANDLE							long	//64bit
#define SOCKADDR_IN					struct sockaddr_in
#define SOCKADDR					struct sockaddr
#define INVALID_SOCKET				-1
#define SOCKET_ERROR				-1
#define SD_BOTH						2

// 함수재정의
#define CopyMemory					memcpy
#define closesocket					close
#define stricmp						strcasecmp

//
#ifndef FIRST_IPADDRESS
#define FIRST_IPADDRESS(x)			((x>>24) & 0xff)
#define SECOND_IPADDRESS(x)			((x>>16) & 0xff)
#define THIRD_IPADDRESS(x)			((x>>8) & 0xff)
#define FOURTH_IPADDRESS(x)			(x & 0xff)
#endif

//////////////////////////////////////////////////////////////////////////
// 해쉬맵 관련 선언
//////////////////////////////////////////////////////////////////////////
namespace __gnu_cxx
{
	template<> 
	struct hash<long long int> 
	{ 
		size_t 
			operator()(long long int __x) const 
		{ return __x; } 
	}; 


	template<> 
	struct hash<long long unsigned int> 
	{ 
		size_t 
			operator()(long long unsigned int __x) const 
		{ return __x; } 
	}; 
}


//////////////////////////////////////////////////////////////////////////
// 함수선언
//////////////////////////////////////////////////////////////////////////
static inline DWORD GetTickCount( )
{
	struct timeval tv;
	struct timezone tz;
	gettimeofday (&tv, &tz);
	return (DWORD)(tv.tv_sec*1000 + tv.tv_usec/1000); // Overflow does not matter.
}

static inline void Sleep(unsigned long msec)
{
	// 결과비교(단위ms)
	// -- Linux --
	// sched_yield()    : 1번=0, 100,000번=63
	// usleep(0)        : 1번=1, 100,000번=측정불가
	// usleep(1)        : 1번=1
	// sleep(0)         : 1번=0, 100,000번=1(이값은 그냥 루프를 100,000번 돌았을때와 동일)
	// -- Windows --
	// Sleep(0)         : 1번=0, 100,000번=63
	// SwitchToThread() : 1번=0, 100,000번=32
	// -- 그외 --
	// 리눅스의 usleep()함수의 해상도는 매우높은편임. 1ms단위까지 정확한듯

	if( msec == 0 )
	{
		sched_yield();
	}
	else
	{
		usleep(msec*1000);
	}
}

static inline void InterlockedIncrement(void volatile *v)
{
	__asm__ __volatile__(
		"lock incl %0"
		:"+m" (((int*)v)[0]));
}

static inline void InterlockedDecrement(void volatile *v)
{
	__asm__ __volatile__(
		"lock decl %0"
		:"+m" (((int*)v)[0]));
}

static inline void Lx_InitRecursiveMutex(pthread_mutex_t *pMutex)
{
	pthread_mutexattr_t	mta;

	// 뮤텍스 속성 설정
	pthread_mutexattr_init(&mta);
	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);

	// 생성
	pthread_mutex_init(pMutex, &mta);
}

static inline bool QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency)
{
	// 해상도는 msec 단위까지만
	lpFrequency->QuadPart = 1000;
	return true;	
}

static inline bool QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount)
{
	struct timeval	tval;
	struct timezone	tzone;
	
	// 주파수 계산
	// 해상도는 msec 단위까지만
	gettimeofday(&tval, &tzone);
	lpPerformanceCount->QuadPart = ((LONGLONG)tval.tv_sec * 1000) + (LONGLONG)(tval.tv_usec/1000);
	
	return true;
}


//////////////////////////////////////////////////////////////////////////
// 바이트 정렬 끝
//////////////////////////////////////////////////////////////////////////
#pragma pack(pop)


//////////////////////////////////////////////////////////////////////////
#endif /* #ifndef WIN32 */


