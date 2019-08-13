//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "common/network/engine/tcp_network_object.h"
#include "rtmp_chunk_stream.h"

//====================================================================================================
// Interface
//====================================================================================================
class IRtmpEncoder : public IObjectCallback
{
	
public:	
	virtual bool OnRtmpEncoderStart(int index_key, uint32_t ip, StreamKey& stream_key) = 0;
	virtual bool OnRtmpEncoderReadyComplete(int index_key, uint32_t ip, StreamKey& stream_key, MediaInfo& media_info) = 0;
	virtual bool OnRtmpEncoderStreamData(int index_key, uint32_t ip, StreamKey& stream_key, std::shared_ptr<FrameInfo>& frame_info) = 0;
};

//====================================================================================================
// RtmpEncoderObject
//====================================================================================================
class RtmpEncoderObject : public TcpNetworkObject, public IRtmpChunkStream
{
public:
					RtmpEncoderObject();
	virtual			~RtmpEncoderObject();
	
public:
	bool			Create(TcpNetworkObjectParam *param);
	virtual void	Destroy();
	
	bool			SendPackt(int data_size, uint8_t *data);

	int				RecvHandler(std::shared_ptr<std::vector<uint8_t>>& data);
	
	// IRtmpChunkStream implement
	bool			OnChunkStreamSend(int data_size, char* data);
	bool 			OnChunkStreamStart(StreamKey& stream_key);
	bool 			OnChunkStreamReadyComplete(StreamKey& stream_key, MediaInfo& media_info);
	bool 			OnChunkStreamData(StreamKey& stream_key, std::shared_ptr<FrameInfo>& frame_info);

	virtual bool	StartKeepAliveCheck(uint32_t keepalive_check_time);
	bool			KeepAliveCheck();

private:
	RtmpChunkStream	_rtmp_chunk_stream;
	StreamKey 		_stream_key;
	MediaInfo		_media_info;
	time_t			_last_packet_time;
};
