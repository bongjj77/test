#pragma once

#ifndef VC_EXTRALEAN
	#define VC_EXTRALEAN		// Windows 헤더에서 거의 사용되지 않는 내용을 제외시킵니다.
#endif

#include <stdio.h>
#include <iostream>

#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
	#define NOMINMAX
	#include <windows.h>
	#include <winsock2.h>
#else
	#include <stdarg.h>
#endif

//====================================================================================================
// 메모리 릭 관련(Windows 디버깅 모드 정의) 
//====================================================================================================
#ifdef WIN32
	#ifdef _DEBUG 
		#define _CRTDBG_MAP_ALLOC
		#define _CRTDBG_MAP_ALLOC_NEW
		#include <crtdbg.h>
		#define CHECK_MEMORY_LEAK		_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
		#define CHECK_BREAK_POINT(a)	_CrtSetBreakAlloc( a );
	#else
		#define CHECK_MEMORY_LEAK
		#define CHECK_BREAK_POINT(a)
	#endif  
#else
	#define CHECK_MEMORY_LEAK7
	#define CHECK_BREAK_POINT(a)
#endif 

#ifndef MAX_PATH
	#define MAX_PATH (256)
#endif 


//====================================================================================================
// Windows/Linux 관련 정의 
//====================================================================================================
#ifdef WIN32
	#define _STDINT_H
	#pragma warning(disable:4200)	// zero-sized array warning
	#pragma warning(disable:4996)	// ( vsprintf, strcpy, fopen ) function unsafe
	#pragma warning(disable:4715)	// return value not exist
	#pragma warning(disable:4244)
	#pragma warning(disable:4717)
	#pragma warning(disable:4018)	// signed/unsigned mismatch
	#pragma warning(disable:4005)	// macro redefinition
	#pragma warning(disable:94)		// the size of an array must be greater than zero
	
#else	
	#include <errno.h>
	extern int errno;
	#include "linux_definition.h"
#endif

//====================================================================================================
// Config 관련 정의
//====================================================================================================
#include "config_define.h"
#include "./common/common_header.h" 
