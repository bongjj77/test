//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "common/network/protocol_object/HttpResponseObject.h"
#include "media/media_define.h"

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

	bool SendPlaylist(StreamKey& streamkey, std::string& playlist);
	bool SendSegmentData(StreamKey& streamkey, std::shared_ptr<std::vector<uint8_t>>& data);

protected:
	virtual bool StartKeepAliveCheck(uint32_t keepalive_check_time);
	bool KeepAliveCheck();

	virtual bool RecvRequest(std::string& request_page, std::string& agent);
	bool RecvPlaylistRequest(std::string& folder, PlaylistType type);
	bool RecvSegmentRequest(std::string& folder, std::string& file_name, SegmentType type);
	bool RecvCrossDomainRequest();

	bool SendContentResponse(std::string content_type, int data_size, char* data, int max_age);

private:

	time_t _last_packet_time;
	uint32_t _keepalive_time;
	std::string	_http_allow_agent;


};
