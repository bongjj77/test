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
					_object_name, _index_key, _remote_ip_string.c_str(), time_gap));
			}

			_is_closeing = true;
			_network_callback->OnNetworkClose(_object_key, _index_key, _remote_ip, _remote_port);
		}
	}

	return true;
}

//====================================================================================================
// Agent check
// - empty : all allow
//====================================================================================================
bool HttpClientObject::AgentCheck(const std::string& agent)
{
	// Agent Check
	if (_http_allow_agent.empty() == false && _http_allow_agent.find(agent) == std::string::npos)
	{
		return false;
	}

	return true;
}

//====================================================================================================
// RequestUrlParsing
// - URL 분리
//  ex) ..../app_name/stream_name/file_name.file_ext?param=param_value
//====================================================================================================
std::shared_ptr<UrlParsInfo> HttpClientObject::UrlPars(const std::string & url)
{
	std::string path;
	std::string param;
	std::vector<std::string> tokens;
	auto pars_info = std::make_shared<UrlParsInfo>(); 

	// path?param
	tokens.clear();
	Tokenize(url, tokens, ("?"));
	if (tokens.size() == 0)
		return nullptr;

	path = tokens[0];
	param = tokens.size() == 2 ? tokens[1] : "";

	// ...../app/stream/file.ext 
	tokens.clear();
	Tokenize(path, tokens, ("/"));
	
	if (tokens.size() < 3)
		return nullptr;

	pars_info->stream_key.first = tokens[tokens.size() - 3]; // app
	pars_info->stream_key.second = tokens[tokens.size() - 2]; // stream
	pars_info->file = tokens[tokens.size() - 1];
 
	// ext
	tokens.clear();
	Tokenize(path, tokens, ("."));
	
	if (tokens.size() != 2)
		return nullptr;

	pars_info->ext = tokens[1];
 
	if (pars_info->ext.compare(HLS_PLAYLIST_EXT) == 0)			pars_info->content_type = HTTP_M3U8_CONTENT_TYPE;
	else if (pars_info->ext.compare(DASH_PLAYLIST_EXT) == 0)	pars_info->content_type = HTTP_TEXT_XML_CONTENT_TYPE;
	else if (pars_info->ext.compare(HLS_SEGMENT_EXT) == 0)		pars_info->content_type = HTTP_VIDE_MPEG_TS_CONTENT_TYPE;
	else if (pars_info->ext.compare(DASH_SEGMENT_EXT) == 0)
	{
		if (pars_info->file.find("audio") >= 0)					pars_info->content_type = HTTP_AUDIO_MP4_CONTENT_TYPE;
		else													pars_info->content_type = HTTP_VIDEO_MP4_CONTENT_TYPE;
	}
	else if (pars_info->ext.compare("xml") == 0)				pars_info->content_type = HTTP_TEXT_XML_CONTENT_TYPE;
	else														pars_info->content_type = HTTP_TEXT_HTML_CONTENT_TYPE;

	return pars_info;
}


//====================================================================================================
// RecvRequest
//====================================================================================================
bool HttpClientObject::RecvRequest(std::string& request_url, std::string& agent)
{
	bool result = false;
	
	_last_packet_time = time(nullptr);

	// Agent Check
	if (AgentCheck(agent) == false)
	{
		LOG_ERROR_WRITE(("[%s] AgentCheck fail - key(%d) ip(%s) agent(%s)",
						_object_name, _index_key, _remote_ip_string.c_str(), agent.c_str()));

		SendErrorResponse("Bad Request");

		return false;
	}

	auto pars_info = UrlPars(request_url);
 
	if (pars_info == nullptr)
	{
		LOG_ERROR_WRITE(("[%s] RecvRequest - UrlPars fail - key(%d) ip(%s) url(%s)",
						_object_name, _index_key, _remote_ip_string.c_str()));

		SendErrorResponse("Bad Request");

		return false;
	}
	 
	if (stricmp(pars_info->file.c_str(), HLS_PLAYLIST_FILE_NAME) == 0)
		result = PlaylistRequest(pars_info, PlaylistType::M3u8);

	else if (stricmp(pars_info->file.c_str(), DASH_PLAYLIST_FILE_NAME) == 0)
		result = PlaylistRequest(pars_info, PlaylistType::Mpd);

	else if (stricmp(pars_info->ext.c_str(), HLS_SEGMENT_EXT) == 0)
		result = SegmentRequest(pars_info, SegmentType::Ts);

	else if (stricmp(pars_info->ext.c_str(), DASH_SEGMENT_EXT) == 0)
		result = SegmentRequest(pars_info, SegmentType::M4s);

	else if (stricmp(pars_info->file.c_str(), "crossdomain.xml") == 0)
		result = CrossDomainRequest();
	else
	{
		LOG_ERROR_WRITE(("[%s] RecvRequest - Page Tokenize Fail - key(%d) ip(%s)", _object_name.c_str(), _index_key, _remote_ip_string.c_str()));
		SendErrorResponse("Not Found");
		return false;
	}

	if (result == false)
	{
		LOG_ERROR_WRITE(("[%s] RecvRequest - Page Process Fail - key(%d) ip(%s)", _object_name.c_str(), _index_key, _remote_ip_string.c_str()));
	}

	return result;
}

//====================================================================================================
// PlaylistRequest
//====================================================================================================
bool HttpClientObject::PlaylistRequest(const std::shared_ptr<UrlParsInfo>& pars_info, PlaylistType type)
{
	std::string playlist;
			
	if (!std::static_pointer_cast<IHttpClient>
		(_object_callback)->OnHttpClientPlaylistRequest(_index_key, _remote_ip, pars_info->stream_key, type, playlist))
	{
		LOG_ERROR_WRITE(("[%s] RecvPlaylistRequest - OnHttpClientPlaylistRequest Error - key(%d) ip(%s)",
						_object_name.c_str(), _index_key, _remote_ip_string.c_str()));

		SendErrorResponse("Not Found");
		return false;
	}

	if (!SendPlaylist(pars_info->stream_key, playlist, pars_info->content_type))
	{
		LOG_ERROR_WRITE(("[%s] RecvPlaylistRequest - SendPlaylist Fail - key(%d) ip(%s)",
			_object_name.c_str(), _index_key, _remote_ip_string.c_str()));

		return false;
	}

	return true;
}

//====================================================================================================
// SegmentRequest
//====================================================================================================
bool HttpClientObject::SegmentRequest(const std::shared_ptr<UrlParsInfo>& pars_info, SegmentType type)
{
	std::shared_ptr<std::vector<uint8_t>> data = nullptr;

	if (!std::static_pointer_cast<IHttpClient>
		(_object_callback)->OnHttpClientSegmentRequest(_index_key, _remote_ip, pars_info->file, pars_info->stream_key, type, data))
	{
		LOG_ERROR_WRITE(("[%s] RecvSegmentRequest - OnHttpClientRequest Error - key(%d) ip(%s)",
			_object_name.c_str(), _index_key, _remote_ip_string.c_str()));

		SendErrorResponse("Not Found");
		return false;
	}
	 
	if (!SendSegmentData(pars_info->stream_key, data, pars_info->content_type))
	{
		LOG_ERROR_WRITE(("[%s] RecvSegmentRequest - SendSegmentData Fail - key(%d) ip(%s)",
			_object_name.c_str(), _index_key, _remote_ip_string.c_str()));

		return false;
	}

	return true;
}

//====================================================================================================
// CrossDomainRequest
//====================================================================================================
bool HttpClientObject::CrossDomainRequest()
{
	std::string cross_domain_xml = "<?xml version=\"1.0\"?>\r\n"\
		"<cross-domain-policy>\r\n"\
		"	<allow-access-from domain=\"*\"/>\r\n"\
		"	<site-control permitted-cross-domain-policies=\"all\"/>\r\n"\
		"</cross-domain-policy>";

	if (SendContentResponse(HTTP_TEXT_XML_CONTENT_TYPE, cross_domain_xml) == false)
	{
		LOG_ERROR_WRITE(("[%s] RecvCrossDomainRequest - crossdomain.xml Data Send Fail - key(%d) ip(%s)",
			_object_name.c_str(), _index_key, _remote_ip_string.c_str()));

		return false;
	}

	return true;
}

//====================================================================================================
// SendPlaylist 
//====================================================================================================
bool HttpClientObject::SendPlaylist(const StreamKey& streamkey, const std::string& playlist, const std::string& content_type)
{
	if (!SendContentResponse(content_type, playlist, 1))
	{
		LOG_ERROR_WRITE(("[%s] SendPlayListData - SendResponse Fail - key(%d) ip(%s)", 
			_object_name.c_str(), _index_key, _remote_ip_string.c_str()));
		return false;
	}

	return true;
}

//====================================================================================================
// SendSegmentData
//====================================================================================================
bool HttpClientObject::SendSegmentData(const StreamKey& streamkey, const std::shared_ptr<std::vector<uint8_t>>& data, const std::string& content_type)
{
	if (!SendContentResponse(content_type, data, 30))
	{
		LOG_ERROR_WRITE(("[%s] SendSegmentData - SendContentResponse Fail - key(%d) ip(%s)", 
			_object_name.c_str(), _index_key, _remote_ip_string.c_str()));
		return false;
	}

	return true;
}
 