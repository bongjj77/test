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
	_framerate_check = true;
	_framerate_check_interval = 10;
	_framerate_check_tick = GetCurrentTick();
	_video_frame_count = 0;
	_audio_frame_count = 0;
	_last_audio_timestamp = 0;

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
		LOG_ERROR_WRITE(("[%s] OnChunkStreamSend - Post Sned Fail - ip(%s)", _object_name.c_str(), _remote_ip_string.c_str()));
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
		LOG_ERROR_WRITE(("[%s] OnChunkStreamStart - OnRtmpProviderStart - ip(%s)", _object_name.c_str(), _remote_ip_string.c_str()));
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
		LOG_ERROR_WRITE(("[%s] OnChunkStreamReadyComplete - OnRtmpProviderReadyComplete - ip(%s)", _object_name.c_str(), _remote_ip_string.c_str()));
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
		LOG_ERROR_WRITE(("[%s] OnChunkStreamData - OnRtmpProviderStreamData - ip(%s)", _object_name.c_str(), _remote_ip_string.c_str()));
		return false;
	}

	// audio time stamp check
	/*
	if (frame->type == FrameType::AudioFrame)
	{
		if (_last_audio_timestamp == 0)
			_last_audio_timestamp = frame->timestamp;


		if (_last_audio_timestamp > frame->timestamp)
		{
			return true;
		}

		_last_audio_timestamp = frame->timestamp;

	}
	*/

	if (_framerate_check == true)
	{
		auto current_tick = GetCurrentTick();

		if (_framerate_check_tick == 0)
			_framerate_check_tick = current_tick;

		if (frame->type == FrameType::AudioFrame) _audio_frame_count++;
		else _video_frame_count++;

		if ((GetCurrentTick() - _framerate_check_tick) >= (_framerate_check_interval * 1000))
		{
			LOG_DEBUG_WRITE(("[%s] Framerate - key(%d) video(%.3f) audio(%.3f)", 
				_object_name.c_str(), _index_key, 
				(double)_video_frame_count / _framerate_check_interval,
				(double)_audio_frame_count/ _framerate_check_interval));
			
			_audio_frame_count = 0;
			_video_frame_count = 0;
			_framerate_check_tick = current_tick;
		}
	}

	return true;
}

//====================================================================================================
// KeepAlive Check  
//====================================================================================================
bool RtmpEncoderObject::StartKeepAliveCheck(uint32_t keepalive_check_time)
{
	//KeepAlive  
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
				LOG_INFO_WRITE(("[%s] KeepAlive TimeOver Remove - key(%d) ip(%s) Gap(%d)", _object_name.c_str(), _index_key, _remote_ip_string.c_str(), time_gap));
			}

			if (_network_callback != nullptr)
				_network_callback->OnNetworkClose(_object_key, _index_key, _remote_ip, _remote_port);
		}
	}

	return true;
}
