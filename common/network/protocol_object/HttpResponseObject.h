﻿//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "../engine/tcp_network_object.h"
#include <map>

#define HTTP_ERROR_BAD_REQUEST 				(400)
#define HTTP_ERROR_NOT_FOUND				(404)
#define HTTP_TEXT_HTML_CONTENT_TYPE 		"text/html"
#define HTTP_TEXT_XML_CONTENT_TYPE 			"text/xml"
#define HTTP_IMAGE_JPG_CONTENT_TYPE 		"image/jpeg"
#define HTTP_IMAGE_GIF_CONTENT_TYPE 		"image/gif"
#define HTTP_APPLICATION_TEXT_CONTENT_TYPE 	"application/text"
#define HTTP_VIDEO_MP4_CONTENT_TYPE			"video/mp4"
#define HTTP_AUDIO_MP4_CONTENT_TYPE			"audio/mp4"
#define HTTP_M3U8_CONTENT_TYPE				"application/vnd.apple.mpegURL"
#define HTTP_VIDE_MPEG_TS_CONTENT_TYPE		"video/MP2T"

//====================================================================================================
// HttpResponseObject
//====================================================================================================
class HttpResponseObject : public TcpNetworkObject
{
public:
	HttpResponseObject();
	virtual ~HttpResponseObject();

public:
	bool IsComplete() { return _is_complete; }
	virtual bool SendContentResponse(const std::string& content_type, const std::string& body, int max_age = 0);
	virtual bool SendContentResponse(const std::string& content_type, const std::shared_ptr<std::vector<uint8_t>>& body, int max_age = 0);
	virtual bool SendResponse(const std::string& header, const std::shared_ptr<std::vector<uint8_t>>& body);
	virtual bool SendErrorResponse(std::string error);
	
	static std::string GetHttpHeaderDateTime();
	void EnableCors(const std::string& origin_url) 
	{ 
		_is_cors_use = true; 
		_cors_origin_url = origin_url;
	}

	void SetCorsOriginList(const std::string& origin_list) { _cors_origin_list = origin_list; }
	void SetCookie(std::string name, std::string value, std::string domain, std::string path);

protected:
	bool HttpFieldParse(const std::string &http_header);
	bool RequestLineParse(const std::string &line);
	bool CorsParse(std::string origin_url);

	virtual int RecvHandler(std::shared_ptr<std::vector<uint8_t>>& data);
	virtual bool RecvRequest(std::string& request_url, const std::map<std::string, std::string> &http_field_list) = 0;

protected:
	bool		_is_complete;
	std::string _http_version;
	std::string _request_page;
	std::map<std::string, std::string> _http_field_list;

	bool		_is_cors_use;
	std::string	_cors_origin_url;
	std::string	_cors_origin_full_url;
	bool		_agent_parsing;
	std::string _cors_origin_list;
	std::string _cookie;
};
