#include "stdafx.h"
 #include "http_response_object.h"
#include <string.h>

//====================================================================================================
// Http Header Date Gmt 시간 문자 
//====================================================================================================
std::string  HttpResponseObject::GetHttpHeaderDateTime()
{
	time_t		time_value		= time(nullptr);
	struct tm	time_struct;
	char		time_buffer[100]	= {0,};

	time_struct = *gmtime(&time_value);
	
	strftime(time_buffer, sizeof(time_buffer), "%a, %d %b %Y %H:%M:%S GMT", &time_struct);

	return std::string(time_buffer); 
}


//====================================================================================================
// 생성자  
//====================================================================================================
HttpResponseObject::HttpResponseObject()
{ 
	_is_complete 		= false; 
	_http_version 	= HTTP_VERSION_1_1;
	_cors_use		 	= false;
	_agent_parsing		= false; 
	
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
int HttpResponseObject::RecvHandler(std::shared_ptr<std::vector<uint8_t>> &data)
{
	std::string::size_type	 	header_start_pos		= 0;
	std::string::size_type		header_end_pos		= 0;
	std::string::size_type		line_end_pos 		= 0;
	std::string					header_string;
	std::string					strPageLine;
	int							read_size			= 0;
	std::vector<std::string> 	tokens; 
	bool						is_complete_close	= false; 
	std::string					strAgent; 

	//파라메터 검증 
	if(data == nullptr || data->size() <= 0 || data->size() > HTTP_REQUEST_DATA_MAX_SIZE)
	{
		SendErrorResponse("Bad Request");
		LOG_WRITE(("ERROR : [%s] HttpResponseObject::RecvHandler Param Fail - IndexKey(%d) IP(%s) Size(%d)", 
			_object_name.c_str(), 
			_index_key, 
			_remote_ip_string.c_str(), 
			data->size()));
		return -1; 
	}

	std::string data_string(data->begin(), data->end());
	data_string.push_back(0);
		
	while((header_end_pos = data_string.find(HTTP_HEADER_END_DELIMITER, header_end_pos)) != std::string::npos)
	{
		header_end_pos +=  strlen(HTTP_HEADER_END_DELIMITER); 

		read_size = (int)header_end_pos; 
			
		//Page Line Parsing 
		header_string = data_string.substr(header_start_pos, header_end_pos - header_start_pos - 1);	
		
		if(header_string.empty() == true)
		{
			break;
		}

		//LOG_WRITE(("Http Header : \n%s", header_string.c_str()));

		line_end_pos = header_string.find('\r'); 
		if(line_end_pos == 0 || line_end_pos == std::string::npos)
		{
			break; 
		}
			
		strPageLine = header_string.substr(0,line_end_pos);
		if(strPageLine.empty() == true)
		{
			break; 
		}

		//토큰 파싱 
		tokens.clear(); 
		
		Tokenize(strPageLine, tokens, std::string(" "));
		
		//최소 토큰 개수 확인( GET/[Page]/HTTP) 
		if(tokens.size() < 3)
		{	
			LOG_WRITE(("ERROR : [%s] HttpResponseObject::RecvHandler - HTTP Header Error - IndexKey(%d) IP(%s)", _object_name, _index_key, _remote_ip_string.c_str()));	
			return -1; 
		}

		//버전 정보 저장 
		_http_version = tokens[HTTP_VERSION_INDEX];

		if(_http_version.size() > HTTP_VERSION_MAX_SIZE)
		{
			LOG_WRITE(("ERROR : [%s] HttpResponseObject::RecvHandler - Version Parsing Error - IndexKey(%d) IP(%s) Size(%d:%d)", 
				_object_name.c_str(),
						_index_key, 
						_remote_ip_string.c_str(),
						_http_version.size(), 
						HTTP_VERSION_MAX_SIZE));	
			return -1; 
		}
		
		//Agent 파싱
		if(_agent_parsing == true)
		{
			std::string::size_type nAgentLineStartPos = header_string.find("User-Agent: ");

			if(nAgentLineStartPos != std::string::npos)
			{
				std::size_t nAgentLineEndPos = header_string.find("\r\n", nAgentLineStartPos); 

				if(nAgentLineEndPos != std::string::npos)
				{
					nAgentLineStartPos += strlen("User-Agent: ");
					
					if(nAgentLineEndPos > nAgentLineStartPos)
					{
						strAgent = header_string.substr(nAgentLineStartPos, nAgentLineEndPos - nAgentLineStartPos);
					}
				}
			}
		}

		// CORS Origin 파싱
		if(_cors_origin_list.empty() == false)
		{
			std::string::size_type nCorsOriginStartPos = header_string.find("Origin:");

			if(nCorsOriginStartPos != std::string::npos)
			{
				std::size_t nCorsOriginEndPos = header_string.find("\r\n", nCorsOriginStartPos); 

				if(nCorsOriginEndPos != std::string::npos)
				{
					nCorsOriginStartPos += strlen("Origin:");
					
					if(nCorsOriginEndPos > nCorsOriginStartPos)
					{
						std::string strOriginURL = header_string.substr(nCorsOriginStartPos, nCorsOriginEndPos - nCorsOriginStartPos);

						ReplaceString(strOriginURL, " ", "");			// 공백 제거
						_cors_origin_full_url = strOriginURL; 
						
						ReplaceString(strOriginURL, "http://", "");		// http:// 제거 
						ReplaceString(strOriginURL, "https://", "");	// https:// 제거 
						 
						
						std::string strFindText = "|" + strOriginURL + "|"; 
						
						if(_cors_origin_list.find(strFindText.c_str()) != std::string::npos)
						{
							EnableCors(strOriginURL.c_str()); // CORS 활성화
						}
					}
				}
			}
		}
		
		
		//Content 처리 
		RecvRequest(tokens[HTTP_REQUEST_PAGE_INDEX], strAgent);

		
		//HTTP1.0 확인 - 처리이후에 서버단에서 연결 종료  
		if(_http_version.find(HTTP_VERSION_1_0) != std::string::npos)
		{
			is_complete_close = true;
		}
		
	
		header_start_pos = header_end_pos;

	}

	_is_complete = (read_size == data->size())? true : false;
	_send_complete_close = is_complete_close;  

	return read_size; 

}

//====================================================================================================
// Error 응답 
//====================================================================================================
bool HttpResponseObject::SendErrorResponse(const char * error_string, int max_age) 
{
	char 		data[4096] 	= {0,};
	char 		body[4096] 	= {0,}; 
	std::string date_time 	= GetHttpHeaderDateTime().c_str();
	
	// HTTP 데이터 설정 
	snprintf(body, sizeof(body), 
					"<html>\n"\
						"<head>\n"\
							"<title>error</title>\n"\
						"</head>\n"\
						"<body>\n"\
							"<br>%s\n"\
						"</body>\n"\
					"</html>", error_string); 

	// HTTP 설정 
	if(_cors_use == false)
	{
		snprintf(data, sizeof(data),
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
						, (int)strlen(body) 
						, body); 
	}
	else
	{
		snprintf(data, sizeof(data),
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
						, (int)strlen(body) 
						, body); 
	}
	
	// 전송 
	if(PostSend(std::make_shared<std::vector<uint8_t>>(data, data + strlen(data))) == false)
	{
		return false; 
	}

	return true; 
}

//====================================================================================================
// Error 응답 
//====================================================================================================
bool HttpResponseObject::SendErrorResponse(const char * error_string) 
{
	char 		data[4096] 	= {0,};
	char 		body[4096] 	= {0,}; 
	std::string date_time 	= GetHttpHeaderDateTime().c_str();
	
	// HTTP 데이터 설정 
	snprintf(body, sizeof(body), 
					"<html>\n"\
						"<head>\n"\
							"<title>error</title>\n"\
						"</head>\n"\
						"<body>\n"\
							"<br>%s\n"\
						"</body>\n"\
					"</html>", error_string); 

	// HTTP 설정 
	if(_cors_use == false)
	{
		snprintf(data, sizeof(data),
						"%s 400 Bad Request\r\n"\
						"Date: %s\r\n"\
						"Server: http server\r\n"\
						"Content-Type: text/html\r\n"\
						"Accept-Ranges: bytes\r\n"\
						"Content-Length: %d\r\n\r\n%s"
						, _http_version.c_str() 
						, date_time.c_str()
						, (int)strlen(body) 
						, body); 
	}
	else
	{
		snprintf(data, sizeof(data),
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
						, (int)strlen(body) 
						, body); 
	}
	
	// 전송 
	return PostSend(std::make_shared<std::vector<uint8_t>>(data, data + strlen(data)));
}



//====================================================================================================
// Http응답 
//====================================================================================================
bool HttpResponseObject::SendResponse(int header_size, char * header, int data_size, char * data)  
{
	bool result = false; 
	
	auto send_data = std::make_shared<std::vector<uint8_t>>(header, header + header_size);
	if (data_size > 0)
	{
		send_data->insert(send_data->end(), data, data + data_size);
	}

	// 전송 
	result = PostSend(send_data);
	
	if(result == false)
	{
		LOG_WRITE(("ERROR : [%s] HttpResponseObject::SendResponse - HTTP Send Fail - IndexKey(%d) IP(%s) Size(%d)", 
			_object_name.c_str(), 
			_index_key, 
			_remote_ip_string.c_str(), 
			send_data->size()));
	}
	
	return result; 
}

//====================================================================================================
// Http응답 
//====================================================================================================
bool HttpResponseObject::SendResponse(std::string content_type, int data_size, char * data)  
{
	if(data == nullptr)
	{
		return false; 
	}
	
	char 		header[4096] 	= {0,};
	std::string date_time 	= GetHttpHeaderDateTime().c_str();

	// HTTP 헤더 설정 
	if(_cors_use == false)
	{
		snprintf(header,	sizeof(header),
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
		snprintf(header,	sizeof(header),
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

	

	return SendResponse((int)strlen(header), header, data_size, data);
	
}


//====================================================================================================
// Redirect Http응답
//====================================================================================================
bool HttpResponseObject::SendRedirectResponse(std::string content_type, std::string strRedirectURL, int data_size, char * data)
{
	char 		header[4096] 	= {0,};
	std::string date_time 	= GetHttpHeaderDateTime().c_str();

	// HTTP 헤더 설정 
	if(_cors_use == false)
	{
		snprintf(header,	sizeof(header),
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
							, strRedirectURL.c_str()
							, data_size); 
	}
	else 
	{
		snprintf(header,	sizeof(header),
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
							, strRedirectURL.c_str()
							, (_is_support_ssl == true) ? "https" : "http"
							, _cors_origin_url.c_str()
							, data_size); 
	
	}
	return SendResponse((int)strlen(header), header, data_size, data);
}


//====================================================================================================
// Redirect Http응답
//====================================================================================================
bool HttpResponseObject::SendRedirectResponse(std::string strRedirectURL)
{
	char 		header[4096] 	= {0,};
	
	// HTTP 헤더 설정 
	if(_cors_use == false)
	{
		snprintf(header,	sizeof(header),
							"%s 302 Found\r\n"\
							"Location: %s\r\n"\
							"Content-Length: 0\r\n\r\n"
							, _http_version.c_str()
							, strRedirectURL.c_str()); 
	}
	else 
	{
		snprintf(header,	sizeof(header),
							"%s 302 Found\r\n"\
							"Location: %s\r\n"\
							"Access-Control-Allow-Credentials: true\r\n"\
							"Access-Control-Allow-Headers: Content-Type, *\r\n"\
							"Access-Control-Allow-Origin: %s://%s\r\n"\
							"Content-Length: 0\r\n\r\n"
							, _http_version.c_str()
							, strRedirectURL.c_str()
							, (_is_support_ssl == true) ? "https" : "http"
							, _cors_origin_url.c_str()); 
	
	}

	return SendResponse((int)strlen(header), header, 0, nullptr);
}


//====================================================================================================
// Cookie 설정
//====================================================================================================
void HttpResponseObject::SetCookie(std::string strName, std::string strValue, std::string strDomain, std::string strPath)
{
	// 기본값 확인
	if(strName.empty() == true || strValue.empty() == true)
	{
		return;
	}

	// 1개 이상 개행
	if(_cookie.empty() == false)
	{
		_cookie += "\r\n";
	}

	// 이름 설정
	_cookie += "Set-Cookie: ";
	_cookie += strName;

	// 값 설정
	_cookie += "=";
	_cookie += strValue;

	// 도메인 설정
	if(strDomain.empty() == false) 
	{
		_cookie += "; Domain=";
		_cookie += strDomain;
	}
	
	// Path 설정
	if(strPath.empty() == false) 
	{
		_cookie += "; Path=";
		_cookie += strPath;
	}
}



