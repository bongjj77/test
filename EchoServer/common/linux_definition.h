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
#define closesocket					close

//
#ifndef FIRST_IPADDRESS
#define FIRST_IPADDRESS(x)			((x>>24) & 0xff)
#define SECOND_IPADDRESS(x)			((x>>16) & 0xff)
#define THIRD_IPADDRESS(x)			((x>>8) & 0xff)
#define FOURTH_IPADDRESS(x)			(x & 0xff)
#endif

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


//////////////////////////////////////////////////////////////////////////
// 바이트 정렬 끝
//////////////////////////////////////////////////////////////////////////
#pragma pack(pop)


//////////////////////////////////////////////////////////////////////////
#endif /* #ifndef WIN32 */


