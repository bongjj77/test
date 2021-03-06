﻿//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "common/thread_timer.h"
#include "common/network/engine/network_manager.h"
#include "rtmp_encoder/rtmp_encoder_manager.h"
#include "http_client/http_client_manager.h"
#include "stream_manager.h" 

enum class NetworkObjectKey
{
	RtmpEncoder,
	HttpClient,
	Max,
};

//====================================================================================================
// Create Param
//====================================================================================================
struct CreateParam
{
	std::string version;
	int			thread_pool_count;
	bool		debug_mode;
	std::string host_name;
	std::string real_host_name;
	int			network_bandwidth; //Kbps
	uint32_t	local_ip;
	int			rtmp_listen_port;
	int			http_listen_port;
	time_t		start_time;
};

//====================================================================================================
// MainObject
//====================================================================================================
class MainObject : public std::enable_shared_from_this<MainObject>,
				public IThreadTimer,
				public ITcpNetwork,
				public IRtmpEncoder,
				public IHttpClient
{
public:
	MainObject(void);
	~MainObject(void);

public:
	bool Create(std::unique_ptr<CreateParam> create_param);
	void Destroy(void);
	bool RemoveNetwork(NetworkObjectKey object_key, int index_key);
	bool RemoveNetwork(NetworkObjectKey object_key, std::vector<int>& index_keys);

	static std::string GetNetworkObjectName(NetworkObjectKey object_key);

private:

	// ITcpNetwork Implement 
	bool OnTcpNetworkAccepted(int object_key, std::shared_ptr<NetTcpSocket> socket, uint32_t ip, int port);

	bool OnTcpNetworkConnected(int object_key,
							NetConnectedResult result,
							std::shared_ptr<std::vector<uint8_t>> connected_param,
							std::shared_ptr<NetTcpSocket> socket,
							unsigned ip,
							int port);	

	int  OnNetworkClose(int object_key, int index_key, uint32_t ip, int port);	

	// IRtmpEncoder implement
	bool OnRtmpEncoderStart(int index_key, uint32_t ip, StreamKey& stream_key);
	bool OnRtmpEncoderReadyComplete(int index_key, uint32_t ip, StreamKey& stream_key, MediaInfo& media_info);
	bool OnRtmpEncoderStreamData(int index_key, uint32_t ip, StreamKey& stream_key, std::shared_ptr<FrameInfo>& frame);

	// IHttpClient implement
	bool OnHttpClientPlaylistRequest(int index_key,
									uint32_t ip,
									const StreamKey& stream_key,
									PlaylistType type,
									std::string& playlist);

	bool OnHttpClientSegmentRequest(int index_key,
									uint32_t ip,
									const std::string& file_name,
									const StreamKey& stream_key,
									SegmentType type,
									std::shared_ptr<std::vector<uint8_t>>& data);


	// Timer callback
	void OnThreadTimer(uint32_t timer_id, bool& delete_timer);

	void GarbageCheckTimerProc();
	void InfoPrintTimerProc();

private:
	ThreadTimer							_timer;
	std::unique_ptr<CreateParam>		_create_param;
	std::shared_ptr<NetworkContextPool>	_network_context_pool = nullptr;
	std::shared_ptr<StreamManager>		_stream_manager = nullptr;
	
	std::shared_ptr<NetworkManager>		_network_table[(int)NetworkObjectKey::Max];
	std::shared_ptr<RtmpEncoderManager>	_rtmp_encoder_manager;
	std::shared_ptr<HttpClientManager>	_http_client_manager;
};