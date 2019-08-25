#include "HttpResponseObject.h"
#include <string.h>

#define HTTP_REQUEST_DATA_MAX_SIZE			(8192)
#define HTTP_REQUEST_PAGE_INDEX				(1)
#define HTTP_VERSION_INDEX					(2)
#define HTTP_VERSION_1_0					("HTTP/1.0")
#define HTTP_VERSION_1_1					("HTTP/1.1")
#define HTTP_VERSION_MAX_SIZE				(50)

#define HTTP_HEADER_END_DELIMITER   		("\r\n\r\n")

//====================================================================================================
// Http Header Date Gmt 시간 문자 
//====================================================================================================
std::string HttpResponseObject::GetHttpHeaderDateTime()
{
	time_t		nTime = time(nullptr);
	struct tm	TimeStruct;
	char		TimeBuffer[100] = { 0, };

	TimeStruct = *gmtime(&nTime);

	strftime(TimeBuffer, sizeof(TimeBuffer), "%a, %d %b %Y %H:%M:%S GMT", &TimeStruct);

	return std::string(TimeBuffer);
}


//====================================================================================================
// 생성자  
//====================================================================================================
HttpResponseObject::HttpResponseObject()
{
	_is_complete = false;
	_http_version = HTTP_VERSION_1_1;
	_is_cors_use = false;
	_agent_parsing = false;

}

//====================================================================================================
// 소멸자   
//====================================================================================================
HttpResponseObject::~HttpResponseObject()
{

}

//====================================================================================================
//  패킷 수신 
//====================================================================================================
int HttpResponseObject::RecvHandler(std::shared_ptr<std::vector<uint8_t>>& data)
{
	std::string string_data;
	char* data_buffer = nullptr;
	std::string::size_type find_pos = 0;
	std::string::size_type header_start_pos = 0;
	std::string::size_type header_end_pos = 0;
	std::string::size_type line_end_pos = 0;
	std::string strHeader;
	std::string page_line;
	int read_size = 0;
	std::vector<std::string> 	tokens;
	bool completed_close = false;
	std::string agent;

	int data_size = data->size(); 

	//파라메터 검증 
	if (data == nullptr || data_size <= 0 || data_size > HTTP_REQUEST_DATA_MAX_SIZE)
	{
		SendErrorResponse("Bad Request");
		LOG_ERROR_WRITE(("[%s] HttpResponseObject::RecvHandler - param Fail - key(%d) ip(%s) size(%d)", 
						_object_name, _index_key, _remote_ip_string, data_size));
		return -1;
	}

	data_buffer = new char[data_size + 1];
	if (data_buffer == nullptr)
	{
		LOG_ERROR_WRITE(("[%s] HttpResponseObject::RecvHandler - dataBuffer is nullptr - key(%d) ip(%s) size(%d)", 
						_object_name, _index_key, _remote_ip_string, data_size));
		return -1;
	}

	memcpy(data_buffer, data->data(), data_size);
	data_buffer[data_size] = 0;

	//데이터 복사 
	string_data = data_buffer;

	//메모리 정리 
	if (data_buffer != nullptr)
	{
		delete[] data_buffer;
		data_buffer = nullptr;
	}

	while ((header_end_pos = string_data.find(HTTP_HEADER_END_DELIMITER, header_end_pos)) != std::string::npos)
	{
		header_end_pos += strlen(HTTP_HEADER_END_DELIMITER);

		read_size = (int)header_end_pos;

		//Page Line Parsing 
		strHeader = string_data.substr(header_start_pos, header_end_pos - header_start_pos - 1);

		if (strHeader.empty() == true)
		{
			break;
		}

		//LOG_WRITE(("Http Header : \n%s", strHeader.c_str()));

		line_end_pos = strHeader.find('\r');
		if (line_end_pos == 0 || line_end_pos == std::string::npos)
		{
			break;
		}

		page_line = strHeader.substr(0, line_end_pos);
		if (page_line.empty() == true)
		{
			break;
		}

		//토큰 파싱 
		tokens.clear();

		Tokenize(page_line, tokens, std::string(" "));

		//최소 토큰 개수 확인( GET/[Page]/HTTP) 
		if (tokens.size() < 3)
		{
			LOG_ERROR_WRITE(("[%s] HttpResponseObject::RecvHandler - HTTP Header Error - IndexKey(%d) IP(%s)", 
				_object_name, _index_key, _remote_ip_string));
			return -1;
		}

		//버전 정보 저장 
		_http_version = tokens[HTTP_VERSION_INDEX];

		if (_http_version.size() > HTTP_VERSION_MAX_SIZE)
		{
			LOG_ERROR_WRITE(("[%s] HttpResponseObject::RecvHandler - Version Parsing Error - IndexKey(%d) IP(%s) Size(%d:%d)",
				_object_name, _index_key, _remote_ip_string, _http_version.size(), HTTP_VERSION_MAX_SIZE));

			return -1;
		}

		//Agent 파싱
		if (_agent_parsing == true)
		{
			std::string::size_type nAgentLineStartPos = strHeader.find("User-Agent: ");

			if (nAgentLineStartPos != std::string::npos)
			{
				std::size_t nAgentLineEndPos = strHeader.find("\r\n", nAgentLineStartPos);

				if (nAgentLineEndPos != std::string::npos)
				{
					nAgentLineStartPos += strlen("User-Agent: ");

					if (nAgentLineEndPos > nAgentLineStartPos)
					{
						agent = strHeader.substr(nAgentLineStartPos, nAgentLineEndPos - nAgentLineStartPos);
					}
				}
			}
		}

		// CORS Origin 파싱
		if (_cors_origin_list.empty() == false)
		{
			std::string::size_type nCorsOriginStartPos = strHeader.find("Origin:");

			if (nCorsOriginStartPos != std::string::npos)
			{
				std::size_t nCorsOriginEndPos = strHeader.find("\r\n", nCorsOriginStartPos);

				if (nCorsOriginEndPos != std::string::npos)
				{
					nCorsOriginStartPos += strlen("Origin:");

					if (nCorsOriginEndPos > nCorsOriginStartPos)
					{
						std::string strOriginURL = strHeader.substr(nCorsOriginStartPos, nCorsOriginEndPos - nCorsOriginStartPos);

						ReplaceString(strOriginURL, " ", "");			// 공백 제거
						_cors_origin_full_url = strOriginURL;

						ReplaceString(strOriginURL, "http://", "");		// http:// 제거 
						ReplaceString(strOriginURL, "https://", "");	// https:// 제거 


						std::string strFindText = "|" + strOriginURL + "|";

						if (_cors_origin_list.find(strFindText.c_str()) != std::string::npos)
						{
							EnableCors(strOriginURL.c_str()); // CORS 활성화
						}
					}
				}
			}
		}


		//Content 처리 
		RecvRequest(tokens[HTTP_REQUEST_PAGE_INDEX], agent);


		//HTTP1.0 확인 - 처리이후에 서버단에서 연결 종료  
		if (_http_version.find(HTTP_VERSION_1_0) != std::string::npos)
		{
			completed_close = true;
		}


		header_start_pos = header_end_pos;

	}

	_is_complete = (read_size == data_size) ? true : false;
	_is_send_completed_close = completed_close;

	return read_size;
}

//====================================================================================================
// Error 응답 
//====================================================================================================
bool HttpResponseObject::SendErrorResponse(const char* error_string, int max_age)
{
	char 		szData[4096] = { 0, };
	char 		szBody[4096] = { 0, };
	std::string date_time = GetHttpHeaderDateTime().c_str();

	// HTTP 데이터 설정 
	snprintf(szBody, sizeof(szBody),
		"<html>\n"\
		"<head>\n"\
		"<title>error</title>\n"\
		"</head>\n"\
		"<body>\n"\
		"<br>%s\n"\
		"</body>\n"\
		"</html>", error_string);

	// HTTP 설정 
	if (_is_cors_use == false)
	{
		snprintf(szData, sizeof(szData),
			"%s 400 Bad Request\r\n"\
			"Date: %s\r\n"\
			"Server: http server\r\n"\
			"Content-Type: text/html\r\n"\
			"Accept-Ranges: bytes\r\n"\
			"Cache-Control: max-age=%d\r\n"\
			"Content-Length: %d\r\n\r\n%s"
			, _http_version.c_str()
			, date_time.c_str()
			, max_age
			, (int)strlen(szBody)
			, szBody);
	}
	else
	{
		snprintf(szData, sizeof(szData),
			"%s 400 Bad Request\r\n"\
			"Date: %s\r\n"\
			"Server: http server\r\n"\
			"Content-Type: text/html\r\n"\
			"Accept-Ranges: bytes\r\n"\
			"Cache-Control: max-age=%d\r\n"\
			"Access-Control-Allow-Credentials: true\r\n"\
			"Access-Control-Allow-Headers: Content-Type, *\r\n"\
			"Access-Control-Allow-Origin: %s://%s\r\n"\
			"Content-Length: %d\r\n\r\n%s"
			, _http_version.c_str()
			, date_time.c_str()
			, max_age
			, (_is_support_ssl == true) ? "https" : "http"
			, _cors_origin_url.c_str()
			, (int)strlen(szBody)
			, szBody);
	}
	
	auto send_data = std::make_shared<std::vector<uint8_t>>(szData, szData + strlen(szData));

	return PostSend(send_data);
}

//====================================================================================================
// Error 응답 
//====================================================================================================
bool HttpResponseObject::SendErrorResponse(const char* error_string)
{
	char 		szData[4096] = { 0, };
	char 		szBody[4096] = { 0, };
	std::string date_time = GetHttpHeaderDateTime().c_str();

	// HTTP 데이터 설정 
	snprintf(szBody, sizeof(szBody),
		"<html>\n"\
		"<head>\n"\
		"<title>error</title>\n"\
		"</head>\n"\
		"<body>\n"\
		"<br>%s\n"\
		"</body>\n"\
		"</html>", error_string);

	// HTTP 설정 
	if (_is_cors_use == false)
	{
		snprintf(szData, sizeof(szData),
			"%s 400 Bad Request\r\n"\
			"Date: %s\r\n"\
			"Server: http server\r\n"\
			"Content-Type: text/html\r\n"\
			"Accept-Ranges: bytes\r\n"\
			"Content-Length: %d\r\n\r\n%s"
			, _http_version.c_str()
			, date_time.c_str()
			, (int)strlen(szBody)
			, szBody);
	}
	else
	{
		snprintf(szData, sizeof(szData),
			"%s 400 Bad Request\r\n"\
			"Date: %s\r\n"\
			"Server: http server\r\n"\
			"Content-Type: text/html\r\n"\
			"Accept-Ranges: bytes\r\n"\
			"Access-Control-Allow-Credentials: true\r\n"\
			"Access-Control-Allow-Headers: Content-Type, *\r\n"\
			"Access-Control-Allow-Origin: %s://%s\r\n"\
			"Content-Length: %d\r\n\r\n%s"
			, _http_version.c_str()
			, date_time.c_str()
			, (_is_support_ssl == true) ? "https" : "http"
			, _cors_origin_url.c_str()
			, (int)strlen(szBody)
			, szBody);
	}

	auto send_data = std::make_shared<std::vector<uint8_t>>(szData, szData + strlen(szData));

	return PostSend(send_data);
}



//====================================================================================================
// Http응답 
//====================================================================================================
bool HttpResponseObject::SendResponse(int header_size, char* header, int data_size, char* data)
{
	bool	result = false;
	char* pBuffer = nullptr;
	int		nBufferSize = 0;

	nBufferSize = header_size + data_size;
	pBuffer = new char[nBufferSize];

	// 헤더 복사 
	memcpy(pBuffer, header, header_size);

	// 데이터 복사 
	if (data_size != 0 && data != nullptr)
	{
		memcpy(pBuffer + header_size, data, data_size);
	}

	auto send_data = std::make_shared<std::vector<uint8_t>>(pBuffer, pBuffer + nBufferSize);
  
	return PostSend(send_data);
}

//====================================================================================================
// Http응답 
//====================================================================================================
bool HttpResponseObject::SendResponse(std::string content_type, int data_size, char* data)
{
	if (data == nullptr)
	{
		return false;
	}

	char 		szHeader[4096] = { 0, };
	std::string date_time = GetHttpHeaderDateTime().c_str();

	// HTTP 헤더 설정 
	if (_is_cors_use == false)
	{
		snprintf(szHeader, sizeof(szHeader),
			"%s 200 OK\r\n"\
			"Date: %s\r\n"\
			"Server: http server\r\n"\
			"Content-Type: %s\r\n"\
			"Accept-Ranges: bytes\r\n"\
			"Content-Length: %d\r\n\r\n"
			, _http_version.c_str()
			, date_time.c_str()
			, content_type.c_str()
			, data_size);
	}
	else
	{
		snprintf(szHeader, sizeof(szHeader),
			"%s 200 OK\r\n"\
			"Date: %s\r\n"\
			"Server: http server\r\n"\
			"Content-Type: %s\r\n"\
			"Accept-Ranges: bytes\r\n"\
			"Access-Control-Allow-Credentials: true\r\n"\
			"Access-Control-Allow-Headers: Content-Type, *\r\n"\
			"Access-Control-Allow-Origin: %s://%s\r\n"\
			"Content-Length: %d\r\n\r\n"
			, _http_version.c_str()
			, date_time.c_str()
			, content_type.c_str()
			, (_is_support_ssl == true) ? "https" : "http"
			, _cors_origin_url.c_str()
			, data_size);
	}

	return SendResponse((int)strlen(szHeader), szHeader, data_size, data);

}

//====================================================================================================
// Redirect Http응답
//====================================================================================================
bool HttpResponseObject::SendRedirectResponse(std::string content_type, std::string redirect_url, int data_size, char* data)
{
	char 		szHeader[4096] = { 0, };
	std::string date_time = GetHttpHeaderDateTime().c_str();

	// HTTP 헤더 설정 
	if (_is_cors_use == false)
	{
		snprintf(szHeader, sizeof(szHeader),
			"%s 302 Found\r\n"\
			"Date: %s\r\n"\
			"Server: http server\r\n"\
			"Content-Type: %s\r\n"\
			"Accept-Ranges: bytes\r\n"\
			"Location: %s\r\n"\
			"Content-Length: %d\r\n\r\n"
			, _http_version.c_str()
			, date_time.c_str()
			, content_type.c_str()
			, redirect_url.c_str()
			, data_size);
	}
	else
	{
		snprintf(szHeader, sizeof(szHeader),
			"%s 302 Found\r\n"\
			"Date: %s\r\n"\
			"Server: http server\r\n"\
			"Content-Type: %s\r\n"\
			"Accept-Ranges: bytes\r\n"\
			"Location: %s\r\n"\
			"Access-Control-Allow-Credentials: true\r\n"\
			"Access-Control-Allow-Headers: Content-Type, *\r\n"\
			"Access-Control-Allow-Origin: %s://%s\r\n"\
			"Content-Length: %d\r\n\r\n"
			, _http_version.c_str()
			, date_time.c_str()
			, content_type.c_str()
			, redirect_url.c_str()
			, (_is_support_ssl == true) ? "https" : "http"
			, _cors_origin_url.c_str()
			, data_size);

	}

	return SendResponse((int)strlen(szHeader), szHeader, data_size, data);
}


//====================================================================================================
// Redirect Http응답
//====================================================================================================
bool HttpResponseObject::SendRedirectResponse(std::string redirect_url)
{
	char 		szHeader[4096] = { 0, };

	// HTTP 헤더 설정 
	if (_is_cors_use == false)
	{
		snprintf(szHeader, sizeof(szHeader),
			"%s 302 Found\r\n"\
			"Location: %s\r\n"\
			"Content-Length: 0\r\n\r\n"
			, _http_version.c_str()
			, redirect_url.c_str());
	}
	else
	{
		snprintf(szHeader, sizeof(szHeader),
			"%s 302 Found\r\n"\
			"Location: %s\r\n"\
			"Access-Control-Allow-Credentials: true\r\n"\
			"Access-Control-Allow-Headers: Content-Type, *\r\n"\
			"Access-Control-Allow-Origin: %s://%s\r\n"\
			"Content-Length: 0\r\n\r\n"
			, _http_version.c_str()
			, redirect_url.c_str()
			, (_is_support_ssl == true) ? "https" : "http"
			, _cors_origin_url.c_str());

	}

	return SendResponse((int)strlen(szHeader), szHeader, 0, nullptr);
}


//====================================================================================================
// Cookie 설정
//====================================================================================================
void HttpResponseObject::SetCookie(std::string name, std::string value, std::string domain, std::string path)
{
	// 기본값 확인
	if (name.empty() == true || value.empty() == true)
	{
		return;
	}

	// 1개 이상 개행
	if (_cookie.empty() == false)
	{
		_cookie += "\r\n";
	}

	// 이름 설정
	_cookie += "Set-Cookie: ";
	_cookie += name;

	// 값 설정
	_cookie += "=";
	_cookie += value;

	// 도메인 설정
	if (domain.empty() == false)
	{
		_cookie += "; Domain=";
		_cookie += domain;
	}

	// Path 설정
	if (path.empty() == false)
	{
		_cookie += "; Path=";
		_cookie += path;
	}
}



