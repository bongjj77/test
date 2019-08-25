#include "HttpRequestObject.h"
#include <string.h>

#define HTTP_HEADER_END_DELIMITER   		("\r\n\r\n")
#define HTTP_OK								("HTTP/1.1 200 OK")
#define HTTP_HEADER_FIELD_SEPERATOR			(":")
#define HTTP_LINE_END   					("\r\n")
#define HTTP_LINE_END_SIZE					(2)
#define HTTP_HEADER_CONTENT_LENGTH_FIELD	("Content-Length")
#define HTTP_NO_LENGTH_INFO_LINE_END		("\r\n0") //("\r\n0\r\n\r\n") 
 
char* strnstr(const char* haystack, const char* needle, size_t length)
{
 	size_t needle_len;
	
	if (0 == (needle_len = strlen(needle)))
		return (char*)haystack;
	
	length = strnlen(haystack, length);

	for (int index = 0; index < (int)(length - needle_len); index++)
	{
		if ((haystack[0] == needle[0]) &&
			(0 == strncmp(haystack, needle, needle_len)))
			return (char*)haystack;

		haystack++;
	}
	return nullptr;
}



//====================================================================================================
// Constructor  
//====================================================================================================
HttpRequestObject::HttpRequestObject()
{
	_is_complete = false;
}

//====================================================================================================
// Destructor   
//====================================================================================================
HttpRequestObject::~HttpRequestObject()
{

}

//====================================================================================================
//  패킷 수신 
//====================================================================================================
int HttpRequestObject::RecvHandler(std::shared_ptr<std::vector<uint8_t>>& data)
{
	char* pLineEnd = nullptr;
	char* pContentLengthField = nullptr;
	char* pContentLength = nullptr;
	char szContentSize[20] = { 0, };
	char* pContent = nullptr;
	int header_size = 0;
	int nContentSize = 0;
	std::string content;

	int data_size = data->size(); 


	if (data == nullptr || data->size() == 0)
	{
		return 0;
	}

	//헤더 수신 확인  확인	
	pContent = strnstr((const char *)data->data(), HTTP_HEADER_END_DELIMITER, data_size);
	if (pContent == nullptr)
	{
		return 0; //데이터 추가 수신 
	}

	pContent += strlen(HTTP_HEADER_END_DELIMITER);
	header_size = pContent - (const char*)data->data();

	// Content-Length 필드 확인  
	pContentLengthField = strnstr((const char*)data->data(), HTTP_HEADER_CONTENT_LENGTH_FIELD, header_size);

	if (pContentLengthField != nullptr)
	{
		// 필드 라인 끝 확인 
		pLineEnd = strstr(pContentLengthField, HTTP_LINE_END);
		if (pLineEnd == nullptr)
		{
			return 0;
		}

		pContentLength = strnstr(pContentLengthField, HTTP_HEADER_FIELD_SEPERATOR, pLineEnd - pContentLengthField);

		if (pContentLength == nullptr)
		{
			return 0;
		}

		pContentLength += strlen(HTTP_HEADER_FIELD_SEPERATOR);

		if ((pLineEnd - pContentLength) <= 0 || (pLineEnd - pContentLength) > 20)
		{
			return 0;
		}

		strncpy(szContentSize, pContentLength, pLineEnd - pContentLength);
		nContentSize = atoi(szContentSize);

		//전체 데이터 수신 확인 
		if (data_size < header_size + nContentSize)
		{
			return 0;
		}

		//데이터 설정 
		content.assign(pContent, nContentSize);
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
		if (data->data()[data_size - 7] != '\r' ||
			data->data()[data_size - 6] != '\n' ||
			data->data()[data_size - 5] != '0' ||
			data->data()[data_size - 4] != '\r' ||
			data->data()[data_size - 3] != '\n' ||
			data->data()[data_size - 2] != '\r' ||
			data->data()[data_size - 1] != '\n')
		{
			return 0;
		}

		nContentSize = data_size - header_size;

		//Chunk-Size 처리
		char* pChunkSizeStart = nullptr;
		char* pChunkSizeEnd = nullptr;
		std::string strChunkSize;
		int 		nChunkSize = 0;
		char* pCurrentPos = pContent - HTTP_LINE_END_SIZE; // [CRLF]한칸 전으로 이동	
		char* pDataEnd = (char *)data->data() + data_size;

		// /r/n[16진수 크기]/r/n
		while (pCurrentPos < pDataEnd)
		{
			pChunkSizeStart = pCurrentPos;
			pChunkSizeEnd = nullptr;

			//Chunk-Size 시작 위치 
			if (pChunkSizeStart[0] != '\r' && pChunkSizeStart[1] != '\n')
			{
				return -1;
			}
			pChunkSizeStart += HTTP_LINE_END_SIZE;

			//Chunk-Size  종료 위치 
			int nMaxSize = (pDataEnd - pCurrentPos);
			for (int nIndex = 0; nIndex < nMaxSize; nIndex++)
			{
				if (nIndex + 1 < nMaxSize && pChunkSizeStart[nIndex] == '\r' && pChunkSizeStart[nIndex + 1] == '\n')
				{
					pChunkSizeEnd = pChunkSizeStart + nIndex;
					break;
				}
			}

			if (pChunkSizeEnd == nullptr)
			{
				return -1;
			}

			//Chunk-Size 문자 설정 
			strChunkSize.assign(pChunkSizeStart, pChunkSizeEnd - pChunkSizeStart);

			//16진수 Chunk-Size 
			nChunkSize = strtol(strChunkSize.c_str(), nullptr, 16);

			if (pCurrentPos + nChunkSize > pDataEnd)
			{
				return -1;
			}

			//Content 복사 
			content.append(pChunkSizeEnd + HTTP_LINE_END_SIZE, nChunkSize);

			// 0 - Chunk End 
			if (nChunkSize == 0)
			{
				break;
			}

			pCurrentPos = pChunkSizeEnd + HTTP_LINE_END_SIZE + nChunkSize;
		}
	}

	//Content 처리 
	RecvContent(content);

	_is_complete = true;

	return header_size + nContentSize;
}

//====================================================================================================
// 채널 정보 요청 (Get방식)
// - HTTP 헤더 구성후 전송 
//====================================================================================================
bool HttpRequestObject::SendRequest(char* request_url, char* host)
{
	char http_data[1024] = { 0, };

	//HTTP 헤더 설정 
	sprintf(http_data, "GET /%s HTTP/1.1\r\n"\
						"Accept: */*\r\n"\
						"Accept-Language: ko\r\n"\
						"User-Agent: Mozilla/4.0\r\n"\
						"Accept-Encoding: gzip, deflate\r\n"\
						"Host: %s\r\n"\
						"Accept-Encoding: \r\n"\
						"Connection: Keep-Alive\r\n\r\n",
						request_url, host);

	auto send_data = std::make_shared<std::vector<uint8_t>>(http_data, http_data + strlen(http_data));

	return PostSend(send_data);
}

//====================================================================================================
// 채널 정보 요청(Post방식) 
// - HTTP 헤더 구성후 전송 
//====================================================================================================
bool HttpRequestObject::SendPostMethodRequest(char* request_url, const char* data, const char* host)
{
	char http_data[4096] = { 0, };

	//HTTP 헤더 설정 
	sprintf(http_data, "POST /%s HTTP/1.1\r\n"\
						"Content-Type: application/x-www-form-urlencoded\r\n"\
						"Host: %s\r\n"\
						"Content-Length: %d\r\n\r\n"\
						"%s",
						request_url, host, strlen(data), data);

	auto send_data = std::make_shared<std::vector<uint8_t>>(http_data, http_data + strlen(http_data));

	return PostSend(send_data);
}