//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "http_client_object.h"

//====================================================================================================
//  Constructor
//====================================================================================================
HttpClientObject::HttpClientObject()
{
	_last_packet_time = time(nullptr);
	_keepalive_time = 0;	
}

//====================================================================================================
// Destructor
//====================================================================================================
HttpClientObject::~HttpClientObject()
{
	Destroy();
}

//====================================================================================================
//  Destroy
//====================================================================================================
void HttpClientObject::Destroy()
{
	
}

//====================================================================================================
// Create
//====================================================================================================
bool HttpClientObject::Create(TcpNetworkObjectParam * param)
{
	if(TcpNetworkObject::Create(param) == false)
	{
		return false;
	}
	
	return true;
}


//====================================================================================================
// Start KeepAlive Check
//====================================================================================================
bool HttpClientObject::StartKeepAliveCheck(uint32_t keepalive_check_time)
{
 	_keepalive_check_time = keepalive_check_time;

	SetKeepAliveCheckTimer((uint32_t)(_keepalive_check_time * 1000 / 2), static_cast<bool (TcpNetworkObject::*)()>(&HttpClientObject::KeepAliveCheck));

	return true;
}

//====================================================================================================
// Keepalive Check
//====================================================================================================
bool HttpClientObject::KeepAliveCheck()
{
	time_t	last_packet_time = _last_packet_time;
	time_t	current_time = time(nullptr);
	int 	time_gap = 0;

	time_gap = current_time - last_packet_time;

	if (time_gap > (int)_keepalive_check_time)
	{
		 
		if (_is_closeing == false && _network_callback != nullptr)
		{
			if (_log_lock == false)
			{
				LOG_INFO_WRITE(("[%s] KeepAlive TimeOver Remove - key(%d) ip(%s) gap(%d)", 
					_object_name, _index_key, _remote_ip_string, time_gap));
			}

			_is_closeing = true;
			_network_callback->OnNetworkClose(_object_key, _index_key, _remote_ip, _remote_port);
		}
	}

	return true;
}


//====================================================================================================
// RecvRequest
//====================================================================================================
bool HttpClientObject::RecvRequest(std::string& request_page, std::string& agent)
{
	bool 						result = false;
	std::vector<std::string> 	tokens;
	std::string					pram;
	std::string					path;
	std::string					file_name;
	std::string					folder;
	std::string					extension;
	std::string::size_type		file_name_pos = 0;
	std::string::size_type		extension_pos = 0;

	// Agent Check
	if (_http_allow_agent.empty() == false)
	{
		if (_http_allow_agent.find(agent) == std::string::npos)
		{

			LOG_ERROR_WRITE(("[%s] RecvRequest - Agent Fail - key(%d) ip(%s) agent(%s)", 
							_object_name, _index_key, _remote_ip_string, agent.c_str()));

			SendErrorResponse("Bad Request");

			return false;
		}
	}
 
	_last_packet_time = time(nullptr);

	if (request_page.empty() == true)
	{
		LOG_ERROR_WRITE(("[%s] RecvRequest - RequestPage Empty - key(%d) ip(%s)",
			_object_name, _index_key, _remote_ip_string));

		SendErrorResponse("Bad Request");

		return false;
	}

	if (request_page.size() > 4000 || request_page.size() < 3)
	{
		LOG_ERROR_WRITE(("[%s] RecvRequest - RequestPage Size Error - key(%d) ip(%s) size(%d)",
			_object_name, _index_key, _remote_ip_string, request_page.size()));

		SendErrorResponse("Bad Request");

		return false;
	}
 
	tokens.clear();
	Tokenize2(request_page.c_str(), tokens, '?');

	if (tokens.size() <= 0)
	{
		LOG_ERROR_WRITE(("[%s] RecvRequest - RequestPage Tokenize Size Error - key(%d) ip(%s)",
				_object_name, _index_key, _remote_ip_string));

		SendErrorResponse("Bad Request");

		return false;
	}
 
	path = tokens[0];
 
	if (tokens.size() >= 2)
	{
		pram = tokens[1];
	}

	// file name
	file_name_pos = path.rfind('/');
	if ((file_name_pos == std::string::npos) || (file_name_pos >= (path.size() - 1)))
	{
		LOG_ERROR_WRITE(("[%s] RecvRequest - FileName Parsing Fail - key(%d) ip(%s)", _object_name, _index_key, _remote_ip_string));
		SendErrorResponse("Bad Request");
		return false;
	}
	file_name = path.substr(file_name_pos + 1);
	folder = path.substr(0, file_name_pos);

	if (file_name.empty() == true)
	{
		LOG_ERROR_WRITE(("[%s] RecvRequest - FileName Empty Error - key(%d) ip(%s)", _object_name, _index_key, _remote_ip_string));
		SendErrorResponse("Bad Request");
		return false;
	}

	// externsion
	extension_pos = file_name.rfind('.');
	if ((extension_pos == std::string::npos) || (extension_pos >= (file_name.size() - 1)))
	{
		LOG_ERROR_WRITE(("[%s] RecvRequest - FileExtension Parsing Fail - key(%d) ip(%s)", _object_name, _index_key, _remote_ip_string));
		SendErrorResponse("Bad Request");
		return false;
	}
	extension = file_name.substr(extension_pos + 1);

	if (extension.empty() == true)
	{
		LOG_ERROR_WRITE(("[%s] RecvRequest - FileExtension Empty() - key(%d) ip(%s)", _object_name, _index_key, _remote_ip_string));
		SendErrorResponse("Bad Request");
		return false;
	}

	if (stricmp(file_name.c_str(), "playlist.m3u8") == 0)  		
		result = RecvPlaylistRequest(folder, PlaylistType::M3u8);

	else if (stricmp(file_name.c_str(), "manifest.mpd") == 0)  	
		result = RecvPlaylistRequest(folder, PlaylistType::Mpd);

	else if (stricmp(extension.c_str(), "ts") == 0) 			
		result = RecvSegmentRequest(folder, file_name, SegmentType::Ts);

	else if (stricmp(extension.c_str(), "m4s") == 0) 			
		result = RecvSegmentRequest(folder, file_name, SegmentType::M4s);	

	else if (stricmp(file_name.c_str(), "crossdomain.xml") == 0)
		result = RecvCrossDomainRequest();

	else
	{
		LOG_ERROR_WRITE(("[%s] RecvRequest - Page Tokenize Fail - key(%d) ip(%s)", _object_name, _index_key, _remote_ip_string));
		SendErrorResponse("Not Found");
		return false;
	}

	if (result == false)
	{
		LOG_ERROR_WRITE(("[%s] RecvRequest - Page Process Fail - key(%d) ip(%s)", _object_name, _index_key, _remote_ip_string));
	}

	return result;
}

//====================================================================================================
// RecvPlaylistRequest
//====================================================================================================
bool HttpClientObject::RecvPlaylistRequest(std::string& folder, PlaylistType type)
{
	StreamKey stream_key;
	std::string playlist;
	std::vector<std::string> tokens;
	std::string app_name;
	std::string stream_name;
 
	tokens.clear();
	Tokenize2(folder.c_str(), tokens, '/');

	if (tokens.size() < 2)
	{
		return false;
	}

	app_name = tokens[tokens.size() - 2];
	stream_name = tokens[tokens.size() - 1];

	stream_key.first = app_name;
	stream_key.second = stream_name;
	
	if (!std::static_pointer_cast<IHttpClient>
		(_object_callback)->OnHttpClientPlaylistRequest(_index_key, _remote_ip, stream_key, type, playlist))
	{
		LOG_ERROR_WRITE(("[%s] RecvPlaylistRequest - OnHttpClientPlaylistRequest Error - key(%d) ip(%s)",
				_object_name, _index_key, _remote_ip_string));

		SendErrorResponse("Not Found");
		return false;
	}

	if (!SendPlaylist(stream_key, playlist))
	{
		LOG_ERROR_WRITE(("[%s] RecvPlaylistRequest - SendPlaylist Fail - key(%d) ip(%s)",
				_object_name, _index_key, _remote_ip_string));

		return false;
	}

	return true;
}

//====================================================================================================
// RecvSegmentRequest
//====================================================================================================
bool HttpClientObject::RecvSegmentRequest(std::string& folder, std::string& file_name, SegmentType type)
{
	StreamKey stream_key;
	std::vector<std::string> tokens;
	std::string app_name;
	std::string stream_name;
 
	tokens.clear();
	Tokenize2(folder.c_str(), tokens, '/');

	if (tokens.size() < 2)
	{
		return false;
	}

	app_name = tokens[tokens.size() - 2];
	stream_name = tokens[tokens.size() - 1];

	stream_key.first = app_name;
	stream_key.second = stream_name;
	 
	tokens.clear();
	Tokenize2(folder.c_str(), tokens, '/');

	if (tokens.size() < 2)
	{
		return false;
	}

	app_name = tokens[tokens.size() - 2];
	stream_name = tokens[tokens.size() - 1];

	stream_key.first = app_name;
	stream_key.second = stream_name;
 
	std::shared_ptr<std::vector<uint8_t>> data = nullptr;

	if (!std::static_pointer_cast<IHttpClient>
		(_object_callback)->OnHttpClientSegmentRequest(_index_key, _remote_ip, file_name, stream_key, type, data))
	{
		LOG_ERROR_WRITE(("[%s] RecvSegmentRequest - OnHttpClientRequest Error - key(%d) ip(%s)",
				_object_name, _index_key, _remote_ip_string));
		SendErrorResponse("Not Found");
		return false;
	}
	 
	if (!SendSegmentData(stream_key, data))
	{
		LOG_ERROR_WRITE(("[%s] RecvSegmentRequest - SendSegmentData Fail - key(%d) ip(%s)",
				_object_name, _index_key, _remote_ip_string));
		return false;
	}

	return true;
}

//====================================================================================================
// RecvCrossDomainRequest
//====================================================================================================
bool HttpClientObject::RecvCrossDomainRequest()
{
	std::string cross_domain_xml = "<?xml version=\"1.0\"?>\r\n"\
		"<cross-domain-policy>\r\n"\
		"	<allow-access-from domain=\"*\"/>\r\n"\
		"	<site-control permitted-cross-domain-policies=\"all\"/>\r\n"\
		"</cross-domain-policy>";

	if (SendResponse(HTTP_TEXT_XML_CONTENT_TYPE, cross_domain_xml.size(), (char*)cross_domain_xml.c_str()) == false)
	{
		LOG_ERROR_WRITE(("[%s] RecvCrossDomainRequest - crossdomain.xml Data Send Fail - key(%d) ip(%s)",
				_object_name, _index_key, _remote_ip_string));

		return false;
	}

	return true;
}

//====================================================================================================
// SendPlaylist 
//====================================================================================================
bool HttpClientObject::SendPlaylist(StreamKey& streamkey, std::string& playlist)
{
	if (!SendContentResponse(HTTP_M3U8_CONTENT_TYPE, playlist.size(), (char*)playlist.c_str(), 1))
	{
		LOG_ERROR_WRITE(("[%s] SendPlayListData - SendResponse Fail - key(%d) ip(%s)", _object_name, _index_key, _remote_ip_string));
		return false;
	}

	return true;
}

//====================================================================================================
// SendSegmentData
//====================================================================================================
bool HttpClientObject::SendSegmentData(StreamKey& streamkey, std::shared_ptr<std::vector<uint8_t>>& data)
{
	if (!SendContentResponse(HTTP_VIDE_MPEG_TS_CONTENT_TYPE, data->size(), (char*)data->data(), 30))
	{
		LOG_ERROR_WRITE(("[%s] SendSegmentData - SendContentResponse Fail - key(%d) ip(%s)", _object_name, _index_key, _remote_ip_string));
		return false;
	}

	return true;
}

//====================================================================================================
// SendContentResponse
//====================================================================================================
bool HttpClientObject::SendContentResponse(std::string content_type, int data_size, char* data, int max_age)
{
	if (data == nullptr)
	{
		return false;
	}

	char header[4096] = { 0, };
	std::string date_time = GetHttpHeaderDateTime().c_str();
 
	if (_is_cors_use == false)
	{
		sprintf(header, "%s 200 OK\r\n"\
			"Date: %s\r\n"\
			"Server: http server\r\n"\
			"Content-Type: %s\r\n"\
			"Accept-Ranges: bytes\r\n"\
			"Cache-Control: max-age=%d\r\n"\
			"Content-Length: %d\r\n\r\n"
			, _http_version.c_str()
			, date_time.c_str()
			, content_type.c_str()
			, max_age
			, data_size);
	}
	else
	{
		sprintf(header, "%s 200 OK\r\n"\
			"Date: %s\r\n"\
			"Server: http server\r\n"\
			"Content-Type: %s\r\n"\
			"Accept-Ranges: bytes\r\n"\
			"Cache-Control: max-age=%d\r\n"\
			"Access-Control-Allow-Credentials: true\r\n"\
			"Access-Control-Allow-Headers: Content-Type, *\r\n"\
			"Access-Control-Allow-Origin: http://%s\r\n"\
			"Content-Length: %d\r\n\r\n"
			, _http_version.c_str()
			, date_time.c_str()
			, content_type.c_str()
			, max_age
			, _cors_origin_url.c_str()
			, data_size);
	}

	return HttpResponseObject::SendResponse(strlen(header), header, data_size, data);
}

 