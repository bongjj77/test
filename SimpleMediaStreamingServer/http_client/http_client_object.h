//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "common/network/protocol_object/HttpResponseObject.h"
#include "media/media_define.h"

struct UrlParsInfo
{
	StreamKey stream_key;
	std::string file;
	std::string ext;

	std::string content_type;

};


//====================================================================================================
// Interface
//====================================================================================================
class IHttpClient : public IObjectCallback
{
public:
	virtual bool OnHttpClientPlaylistRequest(int index_key,
											uint32_t ip,
											const StreamKey& stream_key,
											PlaylistType type,
											std::string& playlist) = 0;

	virtual bool OnHttpClientSegmentRequest(int index_key,
											uint32_t ip,
											const std::string& file_name,
											const StreamKey& stream_key,
											SegmentType type,
											std::shared_ptr<std::vector<uint8_t>>& data) = 0;
};

//====================================================================================================
// HttpClientObject
//====================================================================================================
class HttpClientObject : public HttpResponseObject
{
public:
	HttpClientObject();
	virtual ~HttpClientObject();

public:
	bool Create(TcpNetworkObjectParam* param);
	virtual void Destroy();

	bool SendPlaylist(const StreamKey& streamkey, const std::string& playlist, const std::string& content_type);

	bool SendSegmentData(const StreamKey& streamkey, 
						const std::shared_ptr<std::vector<uint8_t>>& data, 
						const std::string& content_type);

protected:
	virtual bool StartKeepAliveCheck(uint32_t keepalive_check_time);
	bool KeepAliveCheck();

	bool AgentCheck(const std::string & agent); 

	std::shared_ptr<UrlParsInfo> UrlPars(const std::string& url);
	
	virtual bool RecvRequest(std::string& request_url, const std::map<std::string, std::string> &http_field_list);
	bool PlaylistRequest(const std::shared_ptr<UrlParsInfo> & parse_info, PlaylistType type);
	bool SegmentRequest(const std::shared_ptr<UrlParsInfo>& parse_info, SegmentType type);
	bool CrossDomainRequest();

private:

	time_t _last_packet_time;
	uint32_t _keepalive_time;
	std::string	_http_allow_agent;


};
