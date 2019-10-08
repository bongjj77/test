//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

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
// http field parse
//====================================================================================================
bool HttpRequestObject::HttpFieldParse(const std::string &http_header)
{
	std::vector<std::string> tokens;
	Tokenize(http_header, tokens, HTTP_LINE_END);

	if (tokens.size() < 1)
	{
		LOG_ERROR_WRITE(("[%s] RecvHandler - http header tokenize fail - key(%d) ip(%s) size(%d)",
			_object_name.c_str(), _index_key, _remote_ip_string.c_str(), http_header.size()));
		return false;
	}

	// header field parse
	_http_field_list.clear();

	for (const std::string &field_line : tokens)
	{
		auto colon_pos = field_line.find(':');
		if (colon_pos != std::string::npos)
		{
			_http_field_list[field_line.substr(0, colon_pos)] = field_line.substr(colon_pos + 2);
		}
	}

	return true;
}


//====================================================================================================
//  패킷 수신 
//====================================================================================================
int HttpRequestObject::RecvHandler(std::shared_ptr<std::vector<uint8_t>>& data)
{
	int header_size = 0;
	int content_size = 0; 

	std::string content_data;

	int data_size = data->size(); 

	if (data == nullptr || data->size() == 0)
	{
		return 0;
	}

	// header size check
	auto content_pos = strnstr((const char *)data->data(), HTTP_HEADER_END_DELIMITER, data_size);

	if (content_pos == nullptr)
	{
		return 0; 
	}
	content_pos += strlen(HTTP_HEADER_END_DELIMITER);
	header_size = content_pos - (const char*)data->data();

	std::string http_header((char *)data->data(), (char *)content_pos +1);
	
	// http field parse
	if (HttpFieldParse(http_header) == false)
	{
		return -1;
	}

	// Content-Length check
	if (_http_field_list.find(HTTP_HEADER_CONTENT_LENGTH_FIELD) != _http_field_list.end())
	{
		content_size = atoi(_http_field_list[HTTP_HEADER_CONTENT_LENGTH_FIELD].c_str());

		// data size check
		if (data_size < header_size + content_size)
		{
			return 0;
		}

		//데이터 설정 
		content_data.assign(content_pos, content_size);
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

		content_size = data_size - header_size;

		//Chunk-Size 처리
		std::string chunk_size_string;
		int			chunk_size = 0;
		char*		current_pos = content_pos - HTTP_LINE_END_SIZE; // [CRLF]한칸 전으로 이동	
		char*		data_end_pos = (char *)data->data() + data_size;

		// /r/n[16진수 크기]/r/n
		while (current_pos < data_end_pos)
		{
			auto chunk_size_start = current_pos;
			char * chunk_size_end = nullptr;

			//Chunk-Size 시작 위치 
			if (chunk_size_start[0] != '\r' && chunk_size_start[1] != '\n')
			{
				return -1;
			}
			chunk_size_start += HTTP_LINE_END_SIZE;

			//Chunk-Size  종료 위치 
			int max_size = (data_end_pos - current_pos);
			for (int nIndex = 0; nIndex < max_size; nIndex++)
			{
				if (nIndex + 1 < max_size && chunk_size_start[nIndex] == '\r' && chunk_size_start[nIndex + 1] == '\n')
				{
					chunk_size_end = chunk_size_start + nIndex;
					break;
				}
			}

			if (chunk_size_end == nullptr)
			{
				return -1;
			}

			//Chunk-Size 문자 설정 
			chunk_size_string.assign(chunk_size_start, chunk_size_end - chunk_size_start);

			//16진수 Chunk-Size 
			chunk_size = strtol(chunk_size_string.c_str(), nullptr, 16);

			if (current_pos + chunk_size > data_end_pos)
			{
				return -1;
			}

			//Content 복사 
			content_data.append(chunk_size_end + HTTP_LINE_END_SIZE, chunk_size);

			// 0 - Chunk End 
			if (chunk_size == 0)
			{
				break;
			}

			current_pos = chunk_size_end + HTTP_LINE_END_SIZE + chunk_size;
		}
	}

	//Content 처리 
	RecvContent(content_data);

	_is_complete = true;

	return header_size + content_size;
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