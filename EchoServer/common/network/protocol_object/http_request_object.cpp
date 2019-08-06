#include "stdafx.h"
#include "http_request_object.h"
#include <string.h>

char * stristr(const char *string,const char *strSearch)
{
     const char *s,*sub;

     for (;*string;string++) 
	 {
          for (sub=strSearch,s=string;*sub && *s && tolower(*s) == tolower(*sub);sub++,s++){;}

		  if (*sub == 0) 
		  {	
		  	return (char *)string;
		  }
     }
     return nullptr;
}

//#ifdef WIN32
char * strnstr(const char *haystack, const char *needle, size_t len)
{
        int i;
        size_t needle_len;

        /* segfault here if needle is not nullptr terminated */
        if (0 == (needle_len = strlen(needle)))
                return (char *)haystack;

        /* Limit the search if haystack is shorter than 'len' */
        len = strnlen(haystack, len);

        for (i=0; i<(int)(len-needle_len); i++)
        {
                if ((haystack[0] == needle[0]) &&
                        (0 == strncmp(haystack, needle, needle_len)))
                        return (char *)haystack;

                haystack++;
        }
        return nullptr;
}
//#endif 


//====================================================================================================
// 생성자  
//====================================================================================================
HttpRequestObject::HttpRequestObject()
{ 
	_is_complete = false; 
}

//====================================================================================================
// 소멸자   
//====================================================================================================
HttpRequestObject::~HttpRequestObject()
{
	
}

//====================================================================================================
//  패킷 수신 
//====================================================================================================
int HttpRequestObject::RecvHandler(char * data, int data_size)
{
	char *		pLineEnd			= nullptr; 
	char *		pContentLengthField = nullptr;
	char *		contents_length		= nullptr;	
	char		szContentSize[20] 	= {0,};
	char *		pContent			= nullptr; 
	int 		header_size			= 0; 
	int 		contents_size		= 0; 
	std::string strContent;
	
	if(data == nullptr || data_size == 0)
	{
		return 0; 
	}
	
	//헤더 수신 확인  확인	
	pContent = strnstr(data, HTTP_HEADER_END_DELIMITER, data_size);
	if(pContent == nullptr)
	{
		return 0; //데이터 추가 수신 
	}
	
	pContent += strlen(HTTP_HEADER_END_DELIMITER); 
	header_size	= pContent - data; 

	// Content-Length 필드 확인  
	pContentLengthField	= strnstr(data, HTTP_HEADER_CONTENT_LENGTH_FIELD, header_size);

	if(pContentLengthField != nullptr)
	{
		// 필드 라인 끝 확인 
		pLineEnd = strstr(pContentLengthField, HTTP_LINE_END);
		if(pLineEnd == nullptr)
		{
			return 0; 
		}
		
		contents_length = strnstr(pContentLengthField, HTTP_HEADER_FIELD_SEPERATOR, pLineEnd - pContentLengthField); 
		
		if(contents_length == nullptr)
		{
			return 0;
		}
		
		contents_length  += strlen(HTTP_HEADER_FIELD_SEPERATOR);
		
		if((pLineEnd - contents_length) <= 0 || (pLineEnd - contents_length) > 20)
		{
			return 0; 
		}
		
		strncpy(szContentSize, contents_length, pLineEnd - contents_length);
		contents_size	= atoi(szContentSize); 
		
		//전체 데이터 수신 확인 
		if(data_size < header_size + contents_size)
		{
			return 0;  
		}

		//데이터 설정 
		strContent.assign(pContent, contents_size);
	}
	//Content - Length 없는 페이지(Content에  Chunk-size 값 주의)
	// - 구조	
	//	  [CRLF]1b55[CRLF]
	//	  [데이터 시작].....[데이터 끝 (0x1b55 바이트)]
	//	  [CRLF]43c[CRLF][데이터 시작].....[데이터 끝 (0x43c 바이트)]
	//	  [CRLF]0[CRLF][CRLF]
	else
	{
		// 마지막 데이터 확인(유니코드는 문자 비교 X) 
		// 차후 파일 다운로드가 필요하면 Chunk Mode 전환이후 파일 다운로드 필요 
		if(	data[data_size - 7] != '\r'	||
			data[data_size - 6] != '\n'	||
			data[data_size - 5] != '0'		||
			data[data_size - 4] != '\r'	||
			data[data_size - 3] != '\n'	||
			data[data_size - 2] != '\r'	||
			data[data_size - 1] != '\n')
		{
			return 0; 
		}
		
		contents_size = data_size - header_size; 

		//Chunk-Size 처리
		char *		chunk_size_start 	= nullptr; 
		char *		chunk_size_end		= nullptr; 
		std::string strChunkSize;		
		int 		chunk_size			= 0; 
		char *		pCurrentPos			= pContent - HTTP_LINE_END_SIZE; // [CRLF]한칸 전으로 이동	
		char *		pDataEnd			= data + data_size; 
		
		// /r/n[16진수 크기]/r/n
		while(pCurrentPos < pDataEnd)
		{
			chunk_size_start = pCurrentPos; 
			chunk_size_end	= nullptr; 

			//Chunk-Size 시작 위치 
			if(chunk_size_start[0] != '\r' &&  chunk_size_start[1] != '\n' )
			{
				return -1;
			}
			chunk_size_start +=  HTTP_LINE_END_SIZE;
			
			//Chunk-Size  종료 위치 
			int max_size = (pDataEnd - pCurrentPos); 
			for(int index = 0 ; index < max_size; index++) 
			{
				if(index +1 < max_size && chunk_size_start[index] == '\r' && chunk_size_start[index +1] == '\n')
				{
					chunk_size_end = chunk_size_start + index; 
					break;
				}
			}

			if(chunk_size_end == nullptr)
			{
				return -1; 
			}
						
			//Chunk-Size 문자 설정 
			strChunkSize.assign(chunk_size_start, chunk_size_end - chunk_size_start);

			//16진수 Chunk-Size 
			chunk_size = strtol(strChunkSize.c_str(), nullptr, 16); 

			if(pCurrentPos + chunk_size > pDataEnd)
			{
				return -1;
			}
			
			//Content 복사 
			strContent.append(chunk_size_end + HTTP_LINE_END_SIZE, chunk_size); 
			
			// 0 - Chunk End 
			if(chunk_size == 0) 
			{
				break; 
			}
			
			pCurrentPos = chunk_size_end + HTTP_LINE_END_SIZE + chunk_size;
		}
	}

	//Content 처리 
	RecvContent(strContent); 

	_is_complete = true; 
	
	return header_size + contents_size;
}

//====================================================================================================
// 채널 정보 요청 (Get방식)
// - HTTP 헤더 구성후 전송 
//====================================================================================================
bool HttpRequestObject::SendRequest(char * request_url, char * host)
{
	std::ostringstream http_data;
			
	//HTTP 헤더 설정 
	http_data << "GET /" << request_url << " HTTP/1.1\r\n"
		<< "Accept: */*\r\n"
		<< "Accept-Language: ko\r\n"
		<< "User-Agent: Mozilla/4.0\r\n"
		<< "Accept-Encoding: gzip, deflate\r\n"
		<< "Host: " << host << "\r\n"
		<< "Accept-Encoding: \r\n"
		<< "Connection: Keep-Alive\r\n\r\n";

	// 전송 
	std::string send_data = http_data.str(); 
	return PostSend(std::make_shared<std::vector<uint8_t>>(send_data.begin(), send_data.end()));
}

//====================================================================================================
// 채널 정보 요청(Post방식) 
// - HTTP 헤더 구성후 전송 
//====================================================================================================
bool HttpRequestObject::SendPostMethodRequest(char * request_url, const char * data, const char * host)
{
	std::ostringstream http_data;
		
	//HTTP 헤더 설정 
	http_data << "POST /" << request_url <<" HTTP/1.1\r\n"\
		<< "Content-Type: application/x-www-form-urlencoded\r\n"\
		<< "Host: " << host << "\r\n"\
		<< "Content-Length: " << strlen(data) << "\r\n\r\n"\
		<< data;

	// 전송 
	std::string send_data = http_data.str();
	return PostSend(std::make_shared<std::vector<uint8_t>>(send_data.begin(), send_data.end()));
}



