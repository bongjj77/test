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
#define HTTP_HEADER_LINE_DELIMITER   		("\r\n")
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
// http field parse
//====================================================================================================
bool HttpResponseObject::HttpFieldParse(const std::string &http_header)
{
	std::vector<std::string> tokens;
	Tokenize(http_header, tokens, HTTP_HEADER_LINE_DELIMITER);

	if (tokens.size() < 1)
	{
		LOG_ERROR_WRITE(("[%s] RecvHandler - http header tokenize fail - key(%d) ip(%s) size(%d)",
			_object_name.c_str(), _index_key, _remote_ip_string.c_str(), http_header.size()));
		return false;
	}

	if (RequestLineParse(tokens[0]) == false)
	{
		LOG_ERROR_WRITE(("[%s] RecvHandler - http request line error - key(%d) ip(%s)",
			_object_name.c_str(), _index_key, _remote_ip_string.c_str()));
		return false;
	}

	tokens.erase(tokens.begin());

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
// http line pars 
// - [GET/POST] [Page] HTTP/[version]
//====================================================================================================
bool HttpResponseObject::RequestLineParse(const std::string &line)
{
	std::vector<std::string> tokens;

	Tokenize(line, tokens, std::string(" "));

	// 최소 토큰 개수 확인( [GET/POST] [Page] HTTP/[version]) 
	if (tokens.size() < HTTP_VERSION_INDEX + 1)
	{
		return false;
	}

	// 버전 정보 저장 
	_http_version = tokens[HTTP_VERSION_INDEX];

	if (_http_version.size() > HTTP_VERSION_MAX_SIZE)
	{
		return false;
	}

	_request_page = tokens[HTTP_REQUEST_PAGE_INDEX];

	return true;
}

//====================================================================================================
// cors parse
//====================================================================================================
bool HttpResponseObject::CorsParse(std::string origin_url)
{
	if (origin_url.empty() == true)
	{
		return false;
	}

	ReplaceString(origin_url, " ", ""); // 공백 제거
	_cors_origin_full_url = origin_url;

	ReplaceString(origin_url, "http://", ""); // http:// 제거 
	ReplaceString(origin_url, "https://", ""); // https:// 제거 

	std::string find_text = "|" + origin_url + "|";

	if (_cors_origin_list.find(find_text.c_str()) != std::string::npos)
	{
		EnableCors(origin_url); // CORS 활성화
	}

	return true;

}

//====================================================================================================
// recv packet
//====================================================================================================
int HttpResponseObject::RecvHandler(std::shared_ptr<std::vector<uint8_t>>& data)
{
 	// data check
	if (data == nullptr || data->size() <= 0 || data->size() > HTTP_REQUEST_DATA_MAX_SIZE)
	{
		SendErrorResponse("Bad Request");

		LOG_ERROR_WRITE(("[%s] RecvHandler - data size fail - key(%d) ip(%s) size(%d)", 
			_object_name.c_str(), _index_key, _remote_ip_string.c_str(), data->size()));
		return -1;
	}

	std::string http_data(data->begin(), data->end());

	auto header_end_pos = http_data.find(HTTP_HEADER_END_DELIMITER);
	if (header_end_pos == std::string::npos)
	{
		return 0;
	}

	header_end_pos += strlen(HTTP_HEADER_END_DELIMITER);

	auto http_header = http_data.substr(0, header_end_pos - 1);

	// http field parse
	if (HttpFieldParse(http_header) == false)
	{
		return -1;
	}
  
	// cors parse
	if (_cors_origin_list.empty() == false && _http_field_list.find("Origin") != _http_field_list.end())
	{
		CorsParse(_http_field_list["Origin"]);
	}

	// process
	RecvRequest(_request_page, _http_field_list);

	// http 1.0 socket close
	if (_http_version.find(HTTP_VERSION_1_0) != std::string::npos)
	{
		_is_send_completed_close = true;
	}

	_is_complete = true;
	 
	return data->size();
}

//====================================================================================================
// Error 응답 
//====================================================================================================
bool HttpResponseObject::SendErrorResponse(std::string error)
{
 	std::ostringstream http_body;
	std::ostringstream http_data;
 
	std::string date_time = GetHttpHeaderDateTime().c_str();
 
	http_body 
		<< "<html>\n"
		<< "<head>\n"
		<< "<title>error</title>\n"
		<< "</head>\n"
		<< "<body>\n"
		<< "<br>" << error << "\n"
		<< "</body>\n"
		<< "</html>";

	http_data 
		<< _http_version << " 400 Bad Request\r\n"
		<< "Server: http server\r\n"
		<< "Content-Type: text/html\r\n"
		<< "Content-Length: " << http_body.str().size() << "\r\n\r\n"
		<< http_body.str();

	const auto & data_string = http_data.str(); 

	auto send_data = std::make_shared<std::vector<uint8_t>>(data_string.begin(), data_string.end());

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
	 
	http_header 
		<< _http_version << " 200 OK\r\n"
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



