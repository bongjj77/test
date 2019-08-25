//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "common/network/engine/tcp_network_object.h"
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
											std::string& play_list) = 0;

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
class HttpClientObject : public TcpNetworkObject
{
public:
	HttpClientObject();
	virtual ~HttpClientObject();

public:
	bool Create(TcpNetworkObjectParam* param);
	virtual void Destroy();

	bool SendPackt(int data_size, uint8_t* data);

	int RecvHandler(std::shared_ptr<std::vector<uint8_t>>& data);
	bool RecvPacketProcess(int data_size, uint8_t* data);

private:

	time_t _last_packet_time;
	uint32_t _keepalive_time;

};
