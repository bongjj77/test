//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "common_function.h"

#ifndef _WIN32

	#include <stdio.h>
	#include <stdarg.h>
	#include <stdlib.h>
	#include <string.h>
	#include <sys/types.h>
	#include <time.h>
	#include <malloc.h>
	#include <unistd.h>
	#include <ctype.h>
	#include <sys/errno.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/socket.h>
	#include <sys/ioctl.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <net/if.h>
	#include <sys/uio.h>
	#include <netdb.h>
	#include <sys/vfs.h>
	#include <math.h>
	#include <signal.h>
	//#include <rpcdce.h>

	struct UUID
	{
	  uint32_t  Data1;
	  uint16_t Data2;
	  uint16_t Data3;
	  uint8_t  Data4[8];
	};

#else 
	#include <io.h>
	#include <Rpc.h>	
#endif

#include <random>
#include <algorithm>


LogWriter g_log_writer("Q");

//====================================================================================================
// 로그 초기화
//====================================================================================================
bool LogInit(const char * file_name, int nBackupHour) 
{
	//MakeDirectory(file_name);

	bool result = 	g_log_writer.Open((char *)file_name);
	
	g_log_writer.SetDailyBackupTime(nBackupHour); 
	
	return result;
}


//====================================================================================================
// 로그 출력 
//====================================================================================================
void LogPrint(const char * pFormat, ...)
{
	
	char log[MAX_LOG_TEXT_SIZE + 100] = {0, };
	struct tm * current_time;
	time_t timer;
	
	timer = time(nullptr);
	current_time = localtime(&timer); // 초 단위의 시간을 분리하여 구조체에 넣기
	
	
	va_list argument;
	va_start(argument,pFormat);

	vsprintf(log, pFormat, argument);

	va_end(argument);

	//화면 출력 
	printf("\r\n[%04d-%02d-%02d %02d:%02d:%02d]\t%s", current_time->tm_year + 1900,
		current_time->tm_mon + 1,
		current_time->tm_mday,
		current_time->tm_hour,
		current_time->tm_min,
		current_time->tm_sec,
		log);

	fflush(stdout);

	//파일 출력 
	g_log_writer.LogWrite(log);
}

//====================================================================================================
// Network Init
//====================================================================================================
void InitNetwork()
{
#ifdef _WIN32
	// Initialize Winsock
	WSADATA wsaData = {0};
	int iResult = 0;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if ( iResult != 0) 
	{
		wprintf(L"WSAStartup failed: %d\n", iResult);
		return;
	}
#else
    signal(SIGPIPE, SIG_IGN); //for write error signal on linux-wise
#endif

}


//====================================================================================================
// 네트워크 종료 
//====================================================================================================
void ReleaseNetwork()
{
#ifdef _WIN32
    WSACleanup();
#endif

}

//====================================================================================================
// string 파싱 
//====================================================================================================
void Tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters)
{
    std::string::size_type delimPos = 0;
	std::string::size_type tokenPos = 0;
	std::string::size_type pos = 0;

	tokens.clear(); 

	if(str.length()<1)
	{
		return;
	}

	while(1)
	{
		delimPos = str.find_first_of(delimiters, pos);
		tokenPos = str.find_first_not_of(delimiters, pos);

		if(std::string::npos != delimPos)
		{
		 	if(std::string::npos != tokenPos)
			{
		   		if(tokenPos<delimPos)
				{
			 		tokens.push_back(str.substr(pos,delimPos-pos));
		   		}
				else
				{
			 		tokens.push_back("");
		   		}
		 	}
			else
			{
		   		tokens.push_back("");
		 	}
		 	pos = delimPos+1;
		} 
		else 
		{
		 	if(std::string::npos != tokenPos)
			{
		   		tokens.push_back(str.substr(pos));
		 	} 
			else 
			{
		   		tokens.push_back("");
		 	}
		 	break;
		}
	}
	return;
    
}

//====================================================================================================
// string 파싱(빈문자열 추출 가능)  
//====================================================================================================
void Tokenize2(const char * pText, std::vector<std::string>& tokens, char delimiter)
{
    do
    {
        const char * pBegin = pText;

        while(*pText != delimiter && *pText)
        {
            pText++;
        }	

        tokens.push_back(std::string(pBegin, pText));
    }while (0 != *pText++);
}



//====================================================================================================
// 시간 문자 
// - timer 0 입력시 현재시간 
//====================================================================================================
void GetStringTime(std::string & time_string, time_t time_value)
{
	struct tm time_struct;
	
	if(time_value == 0)
	{
		time_value = time(nullptr);
	}

	time_struct = *localtime(&time_value); // 초 단위의 시간을 분리하여 구조체에 넣기

	char szBuffer[40] = {0,}; 
	sprintf(szBuffer, "%04d-%02d-%02d %02d:%02d:%02d", time_struct.tm_year + 1900, time_struct.tm_mon + 1, time_struct.tm_mday, time_struct.tm_hour, time_struct.tm_min, time_struct.tm_sec);

	time_string = szBuffer; 
	
}

//====================================================================================================
// 시간 문자 
// - timer 0 입력시 현재시간 
//====================================================================================================
std::string GetStringDate(time_t time_value, const char * pFormat/* = nullptr */)
{
	std::string time_string; 
	struct tm	time_struct;
	
	if(time_value == 0)
	{
		time_value = time(nullptr);
	}

	time_struct = *localtime(&time_value); // 초 단위의 시간을 분리하여 구조체에 넣기

	char szBuffer[100] = {0,}; 

	if(pFormat == nullptr)
	{
		sprintf(szBuffer, "%04d-%02d-%02d", time_struct.tm_year + 1900, time_struct.tm_mon + 1, time_struct.tm_mday);
	}
	else
	{
		sprintf(szBuffer, pFormat, time_struct.tm_year + 1900, time_struct.tm_mon + 1, time_struct.tm_mday);
	}
	
	time_string = szBuffer; 

	return time_string; 
}



//====================================================================================================
// 시간 문자 
// - timer 0 입력시 현재시간 
//====================================================================================================
std::string GetStringTime2(time_t time_value, bool bDate/*= true*/)
{
	std::string time_string; 
	struct tm	time_struct;
	
	if(time_value == 0)
	{
		time_value = time(nullptr);
	}

	time_struct = *localtime(&time_value); // 초 단위의 시간을 분리하여 구조체에 넣기

	char szBuffer[100] = {0,}; 

	if(bDate == true)
	{
		sprintf(szBuffer, "%04d-%02d-%02d %02d:%02d:%02d", time_struct.tm_year + 1900, time_struct.tm_mon + 1, time_struct.tm_mday, time_struct.tm_hour, time_struct.tm_min, time_struct.tm_sec);
	}
	else
	{
		sprintf(szBuffer, "%02d:%02d:%02d", time_struct.tm_hour, time_struct.tm_min, time_struct.tm_sec);
	}
	
	time_string = szBuffer; 

	return time_string; 
}

//====================================================================================================
// IP 문자 얻기  
//====================================================================================================
std::string GetStringIP(uint32_t ip)
{
	struct sockaddr_in addr_in;
	addr_in.sin_addr.s_addr = ip;

	return std::string(inet_ntoa(addr_in.sin_addr));
}


//====================================================================================================
// Replace std::string
//====================================================================================================
void ReplaceString( std::string & text, const char * before, const char * after )
{
	std::string::size_type current_pos = text.find(before);
	std::string::size_type before_length = strlen(before);
	std::string::size_type after_length = strlen(after);

	while(current_pos < std::string::npos )
	{
		text.replace(current_pos, before_length, after);
		current_pos = text.find(before, current_pos + after_length );
	}
}

//====================================================================================================
// 문자 복사 
// - 최대 버퍼 크기 만큼 안전하게 복사 
//====================================================================================================
void StringNCopy(char * pDest, const char * pSource, int nMaxLength)  
{
	strncpy(pDest, pSource, nMaxLength - 1);
	pDest[nMaxLength-1] = '\0'; 
}

//====================================================================================================
//도메인/IP 문자  IP 정보 획득 
//====================================================================================================
uint32_t 	GetAddressIP(const char * pAddress) 
{
	hostent * 	host 	= nullptr; 
	uint32_t  	ip		= 0; 
	//IP 설정 
	ip = inet_addr(pAddress);

	if(ip != INADDR_NONE)
	{
		return ip; 	
	}

	//Host 이름으로 IP설정 
	host = gethostbyname(pAddress);

	if(host == nullptr)
	{
		return 0; 
	}

	ip = *(uint32_t*)host->h_addr_list[0];


	return ip; 
}


//====================================================================================================
//URL 인코딩
//====================================================================================================
int UrlEncode(const char * pInput, char * pOutput)
{
    char 	hex[4];
    char *	buffer;
    int 	nSize = 0;
	
    buffer = pOutput;

    while(*pInput)
    {
        if ((*pInput > 47 && *pInput < 57) || 
            (*pInput > 64 && *pInput < 92) ||
            (*pInput > 96 && *pInput < 123) ||
            *pInput == '-' || *pInput == '.' || *pInput == '_')
        {
            *buffer = *pInput; 
        }
        else
        {
            sprintf(hex, "%%%02X", *pInput);
            strncat(buffer, hex,3);
            *buffer++;
            *buffer++;
            nSize += 2;
        }
        *pInput++;
        *buffer++;
        nSize++;
    }
    return nSize;
}

//====================================================================================================
//URL 디코딩 
//====================================================================================================
void UrlDecode(const char * pInput, char * pOutput)
{
    int index = 0;
    int num = 0;
    int retval = 0;

    while(*pInput)
    {
        if(*pInput == '%')
        {
        	num 	= 0;
            retval 	= 0;
            for (int i = 0; i < 2; i++)
            {
                *pInput++;
                if(*(pInput) < ':')							num = *(pInput) - 48;
                else if(*(pInput) > '@' && *(pInput) < '[')	num = (*(pInput) - 'A')+10;
                else 										num = (*(pInput) - 'a')+10;

                if((16*(1-i)))	num = (num*16);
				
                retval += num;
            }
            pOutput[index] = retval; 
            index++;
        }
        else
        {
            pOutput[index] = *pInput;  
            index++;
        }
        *pInput++;
    }
	pOutput[index] = 0;
}

//====================================================================================================
//Adts 프레임간 타임스템프 추측 간격 
// -SamplesPerFame.
//   AAC : 1024
//   MP3 : 1152 
//====================================================================================================
int GetAudioTimePerFrame(int nSamplesPerSecond, int nTimeScale, int nSamplesPerFrame)
{
	return  ((double)nTimeScale/nSamplesPerSecond)*nSamplesPerFrame; 
}

bool GetUUID(std::string & strUUID)
{
	TCHAR 	szUUID[33] = {0, }; 
	UUID 	uuid;
#ifdef WIN32
	
 	if(::UuidCreate(&uuid) != RPC_S_OK)
	{
		return false; 
	}

	
#else 
	uuid.Data1 		= (uint32_t)rand();	
	uuid.Data2 		= (uint16_t)rand();
	uuid.Data3 		= (uint16_t)rand(); 
	uuid.Data4[0] 	= rand()%256;
	uuid.Data4[1] 	= rand()%256;
	uuid.Data4[2] 	= rand()%256;
	uuid.Data4[3] 	= rand()%256;
	uuid.Data4[4] 	= rand()%256;
	uuid.Data4[5] 	= rand()%256;
	uuid.Data4[6] 	= rand()%256;
	uuid.Data4[7] 	= rand()%256;
#endif 

	sprintf(szUUID, ("%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X"),	uuid.Data1, 
																		uuid.Data2,
																		uuid.Data3, 
																		uuid.Data4[0], 
																		uuid.Data4[1], 
																		uuid.Data4[2], 
																		uuid.Data4[3], 
																		uuid.Data4[4], 
																		uuid.Data4[5], 
																		uuid.Data4[6], 
																		uuid.Data4[7]);


	strUUID = szUUID;


	return true; 

 }


//====================================================================================================
// 빈 문자열인지 확인 
//====================================================================================================
bool IsEmptyString(char * pString) 
{
	return (pString != nullptr && pString[0] == '\0') ? true : false; 
}


//====================================================================================================
// AvcNal 패턴 
//====================================================================================================
bool AvcNalHeaderPatternCheck(char * data, int data_size, int & patten_index, int & patten_size)
{
	int index = 0; 
	
	while((index + 4 <= data_size) && (index < 128))
	{
		if(data[index] == 0 && data[index + 1] == 0 && data[index + 2] == 0 && data[index + 3] == 1) 
		{
			patten_size = 4;
			patten_index = index;
			return true; 
		}
		else if(data[index] == 0 && data[index + 1] == 0 && data[index + 2] == 1)
		{
			patten_size = 3;
			patten_index = index;
			return true;
		}

		index++; 
	}

	return false; 

}

//====================================================================================================
// AvcNal 크기 확인 
//====================================================================================================
bool AvcNalHeaderSizeCheck(bool key_frame, char * data, int data_size, int & header_size, int & sps_size, int & pps_size) 
{
	int 			pattern_index 				= 0; 
	int 			pattern_size01				= 0;
	int 			pattern_size02 				= 0;
	int 			pattern_size03 				= 0;
	char *			data_position				= data; 
	int 			avc_sps_size					= 0; 
	int 			avc_pps_size					= 0;

	//패턴 확인 
	if(AvcNalHeaderPatternCheck(data_position, data_size, pattern_index, pattern_size01) == false)
	{
		return false; 
	}

	if(pattern_index != 0)
	{
		return false;
	}

	//P-Frame 헤더 크기 설정 
	if(key_frame == false) 
	{
		
		header_size = pattern_size01; 
		return true; 
	}
	

	data_position 	+= pattern_size01;
	data_size		-= pattern_size01;

	//패턴 확인 
	if(AvcNalHeaderPatternCheck(data_position, data_size, pattern_index, pattern_size02) == false)
	{
		return false; 
	}

	//SPS 
	avc_sps_size = pattern_index; 
	
	data_position 	+= (pattern_index + pattern_size02);
	data_size		-= (pattern_index + pattern_size02);

	//패턴 확인 
	if(AvcNalHeaderPatternCheck(data_position, data_size, pattern_index, pattern_size03) == false)
	{
		return false; 
	}
	//PPS
	avc_pps_size = pattern_index; 

	//I-Frame 헤더 크기 설정 
	header_size = pattern_size01 + avc_sps_size + pattern_size02 + avc_pps_size + pattern_size03; 
	sps_size	= avc_sps_size; 
	pps_size	= avc_pps_size; 
	
	return true; 
}

//====================================================================================================
// 오디오 데이터 파싱 
//  - 오디오 데이터는 20개 정도의 프레임이 합쳐져서 들어 온다 .
//    ex) Atds + Frame + Adts + Frame........
//====================================================================================================
bool AdtsDataSizeParsing(uint8_t * data, int data_size, int nAdtsHeaderSize, std::vector<int> & FrameSizeInfos)
{
	int nPosition 	= 0; 
	int	frame_size 	= 0; 
		
	while(nPosition + nAdtsHeaderSize < data_size) 
	{
		//syncword 체크 
		if((data[nPosition] != 0xff) || (data[nPosition + 1]>>4 != 0x0f))
		{
			break; 
		}

		//frame size 확인
		frame_size = (data[nPosition + 3]&0x03)*(1<<11) + data[nPosition + 4]*(1<<3) + (data[nPosition + 5] >> 5);


		if(nPosition + frame_size <=  data_size)
		{
			FrameSizeInfos.push_back(frame_size); 
		}
		
		nPosition += frame_size;
	}

	return true; 
}

//====================================================================================================
//CPU 개수 획득 
//====================================================================================================
int GetCpuCount() 
{
#ifdef WIN32
	
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);

	return SystemInfo.dwNumberOfProcessors;
	
#else
	return sysconf( _SC_NPROCESSORS_ONLN );
#endif 

}

//====================================================================================================
//폴더 생성(하위 폴더 포함) 
//====================================================================================================
void MakeDirectory(const char * pPath)
{
	char	szTempPath[256] = {0,}; 
	char *	pPosition		= nullptr;
	strcpy(szTempPath, pPath); // 경로문자열을 복사

	pPosition = szTempPath; // 포인터를 문자열 처음으로

// Windows
#ifdef WIN32
	while((pPosition = strchr(pPosition, '\\')))
	{
		// 루트디렉토리가 아니면	
		if(pPosition > szTempPath && *(pPosition - 1) != ':')
		{ 
			*pPosition = '\0'; 
			CreateDirectory(szTempPath, nullptr);
			*pPosition = '\\';  
		}
		pPosition++; // 포인터를 다음 문자로 이동
	}

// Linux
#else
	while((pPosition = strchr(pPosition, '/')))
	{
		// 루트디렉토리가 아니면	
		if(pPosition > szTempPath && *(pPosition - 1) != ':')
		{ 
			*pPosition = '\0'; 
			mkdir(szTempPath, S_IFDIR | S_IRWXU | S_IRWXG | S_IXOTH | S_IROTH);
			*pPosition = '/';
		}
		pPosition++; // 포인터를 다음 문자로 이동
	}
#endif 

}


//====================================================================================================
// 반올림 
// - 소숫점 1자리 기준 
//====================================================================================================
int RoundAdjust(double nNumber)
{
	return (int)floor(nNumber+0.5);
}

//====================================================================================================
// 파일 존재 확인 
//====================================================================================================
bool FileExist(const char * pFilePath)
{
#if defined(_WIN32) || defined(_WIN64)
    return (_access(pFilePath, 0) != -1) ? true : false;
#else
    return (access( pFilePath, 0 ) != -1) ? true : false;
#endif
}

//====================================================================================================
// Rtmp URL 파싱
//====================================================================================================
bool RtmpUrlParsing(const char * pAddress, std::string & strServer, std::string & strApp, std::string & stream_name, int & port) 
{
	std::string 			strAddress = pAddress;
	std::string				strPort; 
	std::string::size_type 	nStartPos 	= 0;
	std::string::size_type 	nEndPos 	= 0;
	
	ReplaceString(strAddress, "rtmp://", "");//rtmp:// 제거 

	// 서버 
	nStartPos = strAddress.find( "/" );
	if(nStartPos == 0 || nStartPos == std::string::npos)
	{
		return false; 
	}
	strServer = strAddress.substr(0,nStartPos);

	// 스트림 
	nEndPos = strAddress.rfind( "/" );
	if(nEndPos == 0 || nEndPos == std::string::npos)
	{
		return false; 
	}
	stream_name = strAddress.substr(nEndPos+1);
	
	// APP
	strApp = strAddress.substr(nStartPos + 1, nEndPos - (nStartPos+1));
	
	// Port
	nEndPos = strServer.rfind( ":" );
	if(nEndPos == 0 || nEndPos == std::string::npos)
	{
		port = 1935;
	}
	else
	{
		strPort = strServer.substr(nEndPos+1);
		port = atoi(strPort.c_str()); 

		strServer = strServer.substr(0, nEndPos);
	}
	return true;	
}

//====================================================================================================
// Sleep(Millisecond)
//====================================================================================================
void SleepWait(int nMillisecond)
{
#ifdef WIN32
   	Sleep(nMillisecond);
#else
    usleep(nMillisecond * 1000);   // usleep takes sleep time in us (1 millionth of a second)
#endif
}

//====================================================================================================
// GetTicketCount(Millisecond)
//====================================================================================================
uint32_t get_tick_count()
{
    uint32_t tick = 0ul;
#ifdef WIN32
    tick = GetTickCount();
#else
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    tick = (tp.tv_sec*1000ul) + (tp.tv_nsec/1000ul/1000ul);
#endif
    return tick;

}

//====================================================================================================
// GetTickCount64(Millisecond)
// Windows 2008 이상 지원
//====================================================================================================
uint64_t get_tick_count64()
{
    uint64_t tick = 0ull;
#ifdef WIN32
    tick = GetTickCount64();
#else
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    tick = (tp.tv_sec*1000ull) + (tp.tv_nsec/1000ull/1000ull);
#endif
    return tick;
}

//====================================================================================================
// GetAudioSampleIndex
//====================================================================================================
int GetAudioSampleIndex(int samplerate)
{
	int sample_index = 4; 
	
	const int SampleRateTable[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 0, 0, 0, 0}; 

	// Frequence Index 설정 
	for(int index = 0 ; index < 16 ; index++)
	{
		if(SampleRateTable[index] == samplerate)
		{
			sample_index = index; 
			break; 
		}
	}

	return sample_index;
}

//====================================================================================================
// 오디오  프레임 설정 정보 로드 
//<Adts Header>
// AAAAAAAA AAAABCCD EEFFFFGH HHIJKLMM MMMMMMMM MMMOOOOO OOOOOOPP (QQQQQQQQ QQQQQQQQ) 
// Header consists of 7 or 9 bytes (without or with CRC). 
// Letter  Length (bits)  Description  
// A  12  syncword 0xFFF, all bits must be 1  
// B  1  MPEG Version: 0 for MPEG-4, 1 for MPEG-2  
// C  2  Layer: always 0  
// D  1  protection absent, Warning, set to 1 if there is no CRC and 0 if there is CRC  
// E  2  profile, the MPEG-4 Audio Object Type minus 1  
// F  4  MPEG-4 Sampling Frequency Index (15 is forbidden)  
// G  1  private stream, set to 0 when encoding, ignore when decoding  
// H  3  MPEG-4 Channel Configuration (in the case of 0, the channel configuration is sent via an inband PCE)  
// I  1  originality, set to 0 when encoding, ignore when decoding  
// J  1  home, set to 0 when encoding, ignore when decoding  
// K  1  copyrighted stream, set to 0 when encoding, ignore when decoding  
// L  1  copyright start, set to 0 when encoding, ignore when decoding  
// M  13  frame length, this value must include 7 or 9 bytes of header length: FrameLength = (ProtectionAbsent == 1 ? 7 : 9) + size(AACFrame)  
// O  11  Buffer fullness  
// P  2  Number of AAC frames (RDBs) in ADTS frame minus 1, for maximum compatibility always use 1 AAC frame per ADTS frame  
// Q  16  CRC if protection absent is 0  
//====================================================================================================
#define ADTS_HEADER_SIZE	(7)

bool AdtsHeaderParsing(uint8_t * data, int data_size, int & nAdtsHeaderSize, int & sample_index, int & nChannel)
{
	//syncword 체크 
	if((data == nullptr) || (data_size < ADTS_HEADER_SIZE) || (data[0] != 0xff) || (data[1]>>4 != 0x0f))
	{
		return false;
	}

	//Adtds헤더 사이즈 확인 
	nAdtsHeaderSize = 	((data[1]) & 0x01) == 1 ? ADTS_HEADER_SIZE : ADTS_HEADER_SIZE + 2;
	
	//SampleIndex 획득 
	sample_index = (data[2]>>2) & 0xf;

	//Channel 수 획득 
    nChannel = (data[3] >> 6);

	return true; 
}

bool AdtsHeaderParsing(uint8_t * data, int data_size, int & nAdtsHeaderSize, int & sample_index, int & nChannel, int & nFrameCnt, int & nSamplingFrequency)
{
    //syncword 체크 
    if ((data == nullptr) || (data_size < ADTS_HEADER_SIZE) || (data[0] != 0xff) || (data[1] >> 4 != 0x0f))
    {
        return false;
    }

    //Adtds헤더 사이즈 확인 
    nAdtsHeaderSize = ((data[1]) & 0x01) == 1 ? ADTS_HEADER_SIZE : ADTS_HEADER_SIZE + 2;

    //SampleIndex 획득 
    sample_index = (data[2] >> 2) & 0xf;

    //Channel 수 획득 
    nChannel = (data[3] >> 6);

    //Number of AAC frames
    nFrameCnt = (data[6] & 0x3);

    //SampleRate
    const int SamplingFrequencyTable[] = {
        96000, 88200, 64000,
        48000, 44100,
        32000, 24000, 22050, 16000, 12000, 11025, 8000,
        0, 0, 0, 0
    };

    nSamplingFrequency = SamplingFrequencyTable[sample_index];

    return true;
}

//====================================================================================================
// 비디오 프레임 설정 정보 로드 
// Nal Header +  SPS + NalHeader + PPS + NalHeader 구조 
//====================================================================================================
#define AVC_SPS_MAX_SIZE	(512) 
#define AVC_PPS_MAX_SIZE	(512)

bool AvcHeaderParsing(char * data, int data_size, char * pSps, int & sps_size, char * pPps, int & pps_size) 
{
	int 	pattern_index				= 0; 
	int 	pattern_size01				= 0;
	int 	pattern_size02				= 0;
	int 	pattern_size03				= 0;
	char *	data_position				= data; 
	int 	avc_sps_size 				= 0; 
	char	avc_sps[AVC_SPS_MAX_SIZE]	= {0,}; 
	int 	avc_pps_size 				= 0;
	char	avc_pps[AVC_PPS_MAX_SIZE]	= {0,};

	//패턴 확인 
	if(AvcNalHeaderPatternCheck(data_position, data_size, pattern_index, pattern_size01) == false)
	{
		return false; 
	}

	if(pattern_index != 0)
	{
		return false;
	}

	data_position	+= pattern_size01;
	data_size		-= pattern_size01;

	//패턴 확인 
	if(AvcNalHeaderPatternCheck(data_position, data_size, pattern_index, pattern_size02) == false)
	{
		return false; 
	}

	//SPS 
	avc_sps_size = pattern_index; 
	memcpy(avc_sps, data_position, pattern_index); 

	data_position	+= (pattern_index + pattern_size02);
	data_size		-= (pattern_index + pattern_size02);

	//패턴 확인 
	if(AvcNalHeaderPatternCheck(data_position, data_size, pattern_index, pattern_size03) == false)
	{
		return false; 
	}
	
	//PPS
	avc_pps_size = pattern_index; 
	memcpy(avc_pps, data_position, pattern_index); 

	// 결과 저장 
	sps_size = avc_sps_size;
	memcpy(pSps, avc_sps, avc_sps_size); 
	pps_size	= avc_pps_size;
	memcpy(pPps, avc_pps, avc_pps_size); 

	return true; 
}

//====================================================================================================
// Get Local Hostname
//====================================================================================================
int GetLocalAddress(char *pAddress)
{
#ifdef _WIN32
    char ac[80] = {0, };
    if( gethostname(ac, sizeof(ac)) == SOCKET_ERROR ) 
	{
        LOG_WRITE(( "ERR: %d when getting local host name. %s(%d)", WSAGetLastError(), __FUNCTION__, __LINE__ ));
        return -1;
    }
    LOG_WRITE( ("Host name is %s.", ac ));

    struct hostent *phe = gethostbyname(ac);
    if (phe == 0) 
	{
        LOG_WRITE(( "ERR: Yow! Bad host lookup. %s(%d)", __FUNCTION__, __LINE__ ));
        return 1;
    }

    for( int i = 0; phe->h_addr_list[i] != 0; ++i ) 
	{
        struct in_addr addr;
        memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
        strcpy(pAddress, inet_ntoa(addr));
        // ip 하나만....
        break;
    }
#else
    struct ifconf ifc; /* holds IOCTL return value for SIOCGIFCONF */
    int return_val, fd = -1, numreqs = 30, n;
    struct ifreq *ifr; /* points to one interface returned from ioctl */
    
    fd = socket (PF_INET, SOCK_DGRAM, 0);
    if (fd < 0) 
	{
        return -1;
    }
    
    memset (&ifc, 0x00, sizeof(ifc));
    ifc.ifc_buf = nullptr;
    ifc.ifc_len =  sizeof(struct ifreq) * numreqs;
    ifc.ifc_buf = (char *)malloc(ifc.ifc_len);

    /* This code attempts to handle an arbitrary number of interfaces,
       it keeps trying the ioctl until it comes back OK and the size
       returned is less than the size we sent it.
     */
    for( ; ;) 
	{
        ifc.ifc_len = sizeof(struct ifreq) * numreqs;
        ifc.ifc_buf = (char *)realloc(ifc.ifc_buf, ifc.ifc_len);
        
        if ((return_val = ioctl(fd, SIOCGIFCONF, &ifc)) < 0) {
            //perror("SIOCGIFCONF");
            break;
        }
        if (ifc.ifc_len == (int )sizeof(struct ifreq) * numreqs) {
            /* assume it overflowed and try again */
            numreqs += 10;
            continue;
        }
        break;
    }
    
    if (return_val < 0) 
	{
        return -1;
    }

    // loop through interfaces returned from SIOCGIFCONF
    ifr=ifc.ifc_req;
    for(n=0; n < ifc.ifc_len; n+=sizeof(struct ifreq)) 
	{
        // skip loopback address
        if(strcasecmp(ifr->ifr_name, "lo") == 0) 
		{
            ifr++;
            continue;
        }
        LOG_WRITE(( "ifr_name %s", ifr->ifr_name ));

        // Get the Destination Address for this interface
        return_val = ioctl(fd,SIOCGIFDSTADDR, ifr);
        if(return_val == 0 ) {
            if(ifr->ifr_broadaddr.sa_family == AF_INET) 
			{
                struct sockaddr_in
                    *sin = (struct sockaddr_in *)
                    &ifr->ifr_dstaddr;
                
                LOG_WRITE(( "ifr_dstaddr %s", inet_ntoa(sin->sin_addr) ));
                strcpy(pAddress, inet_ntoa(sin->sin_addr));

            }
        }
        ifr++;
    }
    // we don't need this memory any more
    free(ifc.ifc_buf);
    close(fd);
#endif //
    return 0;
}

//====================================================================================================
// Get Local Hostname
//====================================================================================================
bool GetLocalHostName(std::string & host_name)
{
	char local_host_name[256] = {0,};

	if(gethostname(local_host_name, sizeof(local_host_name))  != 0)
	{
		return false;
	}

	host_name = local_host_name;

	return true;
}

//====================================================================================================
// Random String 
//====================================================================================================
std::string RandomString(uint32_t size)
{
	std::string str("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

	std::random_device rd;
	std::mt19937 generator(rd());
	std::shuffle(str.begin(), str.end(), generator);
	std::string random_str = str.substr(0, size);

	return random_str;
}

//====================================================================================================
// Random String 
//====================================================================================================
std::string RandomNumberString(uint32_t size)
{
	std::string str("012345678901234567890123456789");

	std::random_device rd;
	std::mt19937 generator(rd());
	std::shuffle(str.begin(), str.end(), generator);
	std::string random_str = str.substr(0, size);

	return random_str;
}
