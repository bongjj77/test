//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "../engine/tcp_network_object.h"

#define HTTP_ERROR_BAD_REQUEST 				(400)
#define HTTP_ERROR_NOT_FOUND				(404)
#define HTTP_TEXT_HTML_CONTENT_TYPE 		"text/html"
#define HTTP_TEXT_XML_CONTENT_TYPE 			"text/xml"
#define HTTP_IMAGE_JPG_CONTENT_TYPE 		"image/jpeg"
#define HTTP_IMAGE_GIF_CONTENT_TYPE 		"image/gif"
#define HTTP_APPLICATION_TEXT_CONTENT_TYPE 	"application/text"
#define HTTP_VIDEO_MP4_CONTENT_TYPE			"video/mp4"
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
	virtual bool SendResponse(std::string content_type, int data_size, char* data);
	virtual bool SendResponse(int header_size, char* header, int data_size, char* data);
	virtual bool SendErrorResponse(const char* error_string, int max_age);
	virtual bool SendErrorResponse(const char* error_string);
	virtual bool SendRedirectResponse(std::string content_type, std::string redirect_url, int data_size, char* data);
	virtual bool SendRedirectResponse(std::string redirect_url);

	static std::string GetHttpHeaderDateTime();
	void EnableCors(const char* cors_origin_url) { _is_cors_use = true; _cors_origin_url = cors_origin_url; }
	void SetCorsOriginList(const char* cors_origin_list) { _cors_origin_list = cors_origin_list; }
	void SetCookie(std::string name, std::string value, std::string domain, std::string path);

protected:
	virtual int			RecvHandler(std::shared_ptr<std::vector<uint8_t>>& data);
	virtual bool		RecvRequest(std::string& request_page, std::string& agent) = 0;//agent값은 _agent_parsing == true에서 정상값 

protected:
	bool		_is_complete;
	std::string _http_version;

	bool		_is_cors_use;
	std::string	_cors_origin_url;
	std::string	_cors_origin_full_url;
	bool		_agent_parsing;
	std::string _cors_origin_list;
	std::string _cookie;
};
