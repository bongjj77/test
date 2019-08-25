//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "rtmp_encoder_object.h"


//====================================================================================================
//  RtmpEncoderObject
//====================================================================================================
RtmpEncoderObject::RtmpEncoderObject()
{
	_last_packet_time = time(nullptr);
}

//====================================================================================================
//  ~RtmpEncoderObject
//====================================================================================================
RtmpEncoderObject::~RtmpEncoderObject()
{
	Destroy();
}

//====================================================================================================
//  Destroy
//====================================================================================================
void RtmpEncoderObject::Destroy()
{
	_rtmp_chunk_stream.Destroy();
}

//====================================================================================================
// Create
//====================================================================================================
bool RtmpEncoderObject::Create(TcpNetworkObjectParam * param)
{
	if(TcpNetworkObject::Create(param) == false)
	{
		return false;
	}
	
	_rtmp_chunk_stream.Create(this, 4096);

	_last_packet_time = time(nullptr);

	return true;
}

//====================================================================================================
// Packet Send
//====================================================================================================
bool RtmpEncoderObject::SendPackt(int data_size, uint8_t *data)
{
	auto send_data = std::make_shared<std::vector<uint8_t>>(data_size);
	 

	return PostSend(send_data, false);
}

//====================================================================================================
//  Recv Handler
//====================================================================================================
int RtmpEncoderObject::RecvHandler(std::shared_ptr<std::vector<uint8_t>>& data)
{
	_last_packet_time = time(nullptr); 

	return _rtmp_chunk_stream.OnDataReceived(data->size(), (char *)data->data());
}
//====================================================================================================
// NetworkObject  
//====================================================================================================
bool RtmpEncoderObject::OnChunkStreamSend(int data_size, char* data)
{
	auto send_data = std::make_shared<std::vector<uint8_t>>(data, data + data_size);

	if (PostSend(send_data) == false)
	{
		LOG_ERROR_WRITE(("[%s] OnChunkStreamSend - Post Sned Fail - ip(%s)", _object_name, _remote_ip_string));
		return false;
	}

	return true;
}

//====================================================================================================
// RtmpChunkStream 
//====================================================================================================
bool RtmpEncoderObject::OnChunkStreamStart(StreamKey& stream_key)
{
	// Callback 호占쏙옙	
	if (std::static_pointer_cast<IRtmpEncoder>(_object_callback)->OnRtmpEncoderStart(_index_key, _remote_ip, stream_key) == false)
	{
		LOG_ERROR_WRITE(("[%s] OnChunkStreamStart - OnRtmpProviderStart - ip(%s)", _object_name, _remote_ip_string));
		return false;
	}


	_stream_key = stream_key;

	return true;
}

//====================================================================================================
// RtmpChunkStream
//====================================================================================================
bool RtmpEncoderObject::OnChunkStreamReadyComplete(StreamKey& stream_key, MediaInfo& media_info)
{
	// Callback  	
	if (std::static_pointer_cast<IRtmpEncoder>(_object_callback)->OnRtmpEncoderReadyComplete(_index_key, _remote_ip, stream_key, media_info) == false)
	{
		LOG_ERROR_WRITE(("[%s] OnChunkStreamReadyComplete - OnRtmpProviderReadyComplete - ip(%s)", _object_name, _remote_ip_string));
		return false;
	}

	_media_info = media_info;

	return true;
}

//====================================================================================================
// RtmpChunkStream  
// 
//====================================================================================================
bool RtmpEncoderObject::OnChunkStreamData(StreamKey& stream_key, std::shared_ptr<FrameInfo>& frame)
{

	_last_packet_time = time(nullptr);

	// Callback  
	if (std::static_pointer_cast<IRtmpEncoder>(_object_callback)->OnRtmpEncoderStreamData(_index_key, _remote_ip, stream_key, frame) == false)
	{
		LOG_ERROR_WRITE(("[%s] OnChunkStreamData - OnRtmpProviderStreamData - ip(%s)", _object_name, _remote_ip_string));
		return false;
	}

	return true;
}

//====================================================================================================
// KeepAlive Check 占쏙옙占쏙옙 
//====================================================================================================
bool RtmpEncoderObject::StartKeepAliveCheck(uint32_t keepalive_check_time)
{
	//KeepAlive 占쏙옙占쏙옙
	_keepalive_check_time = keepalive_check_time;

	SetKeepAliveCheckTimer((uint32_t)(_keepalive_check_time * 1000 / 2), static_cast<bool (TcpNetworkObject::*)()>(&RtmpEncoderObject::KeepAliveCheck));


	return true;
}

//====================================================================================================
// KeepAlive 
//====================================================================================================
bool RtmpEncoderObject::KeepAliveCheck()
{
	time_t	last_packet_time = _last_packet_time; 
	time_t	current_time = time(nullptr);
	int 	time_gap = 0;

	time_gap = current_time - last_packet_time;

	if (time_gap > _keepalive_check_time)
	{
		// 
		if (_is_closeing == false && _network_callback != nullptr)
		{
			if (_log_lock == false)
			{
				LOG_INFO_WRITE(("[%s] KeepAlive TimeOver Remove - key(%s) ip(%s) Gap(%d)", _object_name, _index_key, _remote_ip_string, time_gap));
			}

			if (_network_callback != nullptr)
				_network_callback->OnNetworkClose(_object_key, _index_key, _remote_ip, _remote_port);
		}
	}

	return true;
}
