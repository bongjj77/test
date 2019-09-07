//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

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
	std::vector<std::string> tokens;
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
 
	string_data = data_buffer;
 
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
			LOG_ERROR_WRITE(("[%s] HttpResponseObject::RecvHandler - HTTP Header Error - key(%s) ip(%s)", 
				_object_name, _index_key, _remote_ip_string));
			return -1;
		}

		//버전 정보 저장 
		_http_version = tokens[HTTP_VERSION_INDEX];

		if (_http_version.size() > HTTP_VERSION_MAX_SIZE)
		{
			LOG_ERROR_WRITE(("[%s] HttpResponseObject::RecvHandler - Version Parsing Error - key(%s) ip(%s) Size(%d:%d)",
				_object_name, _index_key, _remote_ip_string, _http_version.size(), HTTP_VERSION_MAX_SIZE));

			return -1;
		}

		//Agent 파싱
		if (_agent_parsing == true)
		{
			std::string::size_type agent_line_start_pos = strHeader.find("User-Agent: ");

			if (agent_line_start_pos != std::string::npos)
			{
				std::size_t agent_line_end_pos = strHeader.find("\r\n", agent_line_start_pos);

				if (agent_line_end_pos != std::string::npos)
				{
					agent_line_start_pos += strlen("User-Agent: ");

					if (agent_line_end_pos > agent_line_start_pos)
					{
						agent = strHeader.substr(agent_line_start_pos, agent_line_end_pos - agent_line_start_pos);
					}
				}
			}
		}

		// CORS Origin 파싱
		if (_cors_origin_list.empty() == false)
		{
			std::string::size_type origin_start_pos = strHeader.find("Origin:");

			if (origin_start_pos != std::string::npos)
			{
				std::size_t origin_end_pos = strHeader.find("\r\n", origin_start_pos);

				if (origin_end_pos != std::string::npos)
				{
					origin_start_pos += strlen("Origin:");

					if (origin_end_pos > origin_start_pos)
					{
						std::string origin_url = strHeader.substr(origin_start_pos, origin_end_pos - origin_start_pos);

						ReplaceString(origin_url, " ", ""); // 공백 제거
						_cors_origin_full_url = origin_url;

						ReplaceString(origin_url, "http://", ""); // http:// 제거 
						ReplaceString(origin_url, "https://", ""); // https:// 제거 
						
						std::string find_text = "|" + origin_url + "|";

						if (_cors_origin_list.find(find_text.c_str()) != std::string::npos)
						{
							EnableCors(origin_url); // CORS 활성화
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
bool HttpResponseObject::SendErrorResponse(std::string error)
{
 	std::ostringstream http_body;
	std::ostringstream http_data;
 
	std::string date_time = GetHttpHeaderDateTime().c_str();
 
	http_body << "<html>\n"
		<< "<head>\n"
		<< "<title>error</title>\n"
		<< "</head>\n"
		<< "<body>\n"
		<< "<br>" << error << "\n"
		<< "</body>\n"
		<< "</html>";

	http_data << _http_version << " 400 Bad Request\r\n"
		<< "Server: http server\r\n"
		<< "Content-Type: text/html\r\n"
		<< "Content-Length: " << http_body.str().size() << "\r\n\r\n"
		<< http_body.str();

	auto send_data = std::make_shared<std::vector<uint8_t>>(http_data.str().begin(), http_data.str().end());

	return PostSend(send_data);
}

//====================================================================================================
// Http Content Respons
//====================================================================================================
bool HttpResponseObject::SendContentResponse(const std::string& content_type, const std::string& body, int max_age)
{
	auto data = std::make_shared<std::vector<uint8_t>>(body.begin(), body.end());

	return SendContentResponse(content_type, data, max_age);
}

//====================================================================================================
// Http Content Respons
//====================================================================================================
bool HttpResponseObject::SendContentResponse(const std::string& content_type, const std::shared_ptr<std::vector<uint8_t>>& body, int max_age)
{
	if (body == nullptr)
	{
		return false;
	}

	std::ostringstream http_header;
	std::string date_time = GetHttpHeaderDateTime().c_str();

	// HTTP 헤더 설정 
	 
	http_header << _http_version << " 200 OK\r\n"
		<< "Server: http server\r\n"
		<< "Content-Type: " << content_type << "\r\n"
		<< "Date: " << date_time << "\r\n"
		<< "Cache-Control: max-age=" << max_age << "\r\n";

	if (_is_cors_use == true)
	{
		http_header << "Access-Control-Allow-Credentials: true\r\n"
			"Access-Control-Allow-Headers: Content-Type, *\r\n";
		
		//if(_is_support_ssl)
		//	http_header << "Access-Control-Allow-Origin: https://" << _cors_origin_url << "\r\n";
		//else
			http_header << "Access-Control-Allow-Origin: http://" << _cors_origin_url << "\r\n";
	}

	http_header  << "Content-Length: " << body->size() << "\r\n\r\n";

	return HttpResponseObject::SendResponse(http_header.str(), body);
}

//====================================================================================================
// Http Response 
//====================================================================================================
bool HttpResponseObject::SendResponse(const std::string& header, const std::shared_ptr<std::vector<uint8_t>>& body)
{
	bool result = false;

	auto data = std::make_shared<std::vector<uint8_t>>(header.begin(), header.end());

	if (body != nullptr)
	{
		data->insert(data->end(), body->begin(), body->end());
	}

	return PostSend(data);
}

//====================================================================================================
// Cookie Setting
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



