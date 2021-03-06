﻿//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "common/common_function.h"
#include "media/media_define.h"
#include <mutex>
#include <memory>
#include <media/hls/hls_packetizer.h>
#include <media/dash/dash_packetizer.h>

#pragma pack(1)

class MainObject;

enum class StreamStatus
{
	Ready = 0,
	Complete,
	Max,
};

static char g_stream_status_string[(int)StreamStatus::Max][MAX_PATH] =
{
	"Ready",
	"Complete",
};

enum class StreamError
{
	None = 0,
	Provider,
	Publisher,
	HttpClient,
	Unknown,
	Max,
};

static char g_stream_error_string[(int)StreamError::Max][MAX_PATH] =
{
	"None",
	"Provider Error",
	"Publisher Error",
	"HttpClient Error",
	"Unknown Error",
};

//====================================================================================================
// StreamInfo
//====================================================================================================
struct StreamInfo
{
public:

	StreamInfo(const StreamKey& stream_key_, int provider_index_key_, uint32_t provider_ip_)
	{
		stream_key = stream_key_;
		create_time = time(nullptr);
		state_code = StreamStatus::Ready;
		error_code = StreamError::None;
		provider_index_key = provider_index_key_;
		provider_ip = provider_ip_;
		// hls_packetizer = nullptr;
		dash_packetizer = nullptr;
	}

	bool CreatePacketizer(MediaInfo	media_info_, 
						const std::string & segment_prefix_, 
						uint32_t segment_duration_,
						uint32_t segment_count_)
	{
		media_info = media_info_;

		media_info.video_timescale = HLS_TIMESCALE;
		media_info.audio_timescale = HLS_TIMESCALE;
				
		hls_packetizer = std::make_unique<HlsPacketizer>(stream_key.first.c_str(), 
														stream_key.second.c_str(),
														PacketizerStreamingType::Both,
														segment_prefix_,
														segment_duration_,
														segment_count_, 
														media_info);
														
		
		/*
		dash_packetizer = std::make_unique<DashPacketizer>(stream_key.first.c_str(),
														stream_key.second.c_str(),
														PacketizerStreamingType::Both,
														segment_prefix_,
														segment_duration_,
														segment_count_,
														media_info);

		*/
		
		return true;
	}

	bool AppendFrame(std::shared_ptr<FrameInfo>& frame)
	{
		
		if (frame->type != FrameType::AudioFrame)
		{
			auto copy_frame = std::make_shared<FrameInfo>(frame);

			if(hls_packetizer != nullptr)
				hls_packetizer->AppendVideoFrame(copy_frame);
			
			// if(dash_packetizer != nullptr)
			//	dash_packetizer->AppendVideoFrame(frame);
		}
		else
		{
			auto copy_frame = std::make_shared<FrameInfo>(frame);

			if (hls_packetizer != nullptr)
				hls_packetizer->AppendAudioFrame(copy_frame);
			
			// if(dash_packetizer != nullptr)
			// dash_packetizer->AppendAudioFrame(frame);
		}
		
		return true;
	}

public:
	StreamKey		stream_key;
	time_t			create_time;
	StreamStatus	state_code;
	StreamError		error_code;

	int				provider_index_key;
	uint32_t		provider_ip;

	MediaInfo		media_info;
	std::unique_ptr<HlsPacketizer> hls_packetizer; 
	std::unique_ptr<DashPacketizer> dash_packetizer;
};

#pragma pack()

//====================================================================================================
// StreamManager
//====================================================================================================
class StreamManager
{
public:
	StreamManager(std::shared_ptr<MainObject>& main_object);
	virtual ~StreamManager();

public:
	bool CreateStream(const StreamKey& stream_key, int provider_index_key, uint32_t provider_ip);
	bool SetCompleteState(const StreamKey& stream_key, MediaInfo& media_info);
	
	bool Remove(const StreamKey& stream_key);
	void RemoveAll();
	bool SetError(const StreamKey& stream_key, StreamError error_code);
	bool SetStatus(const StreamKey& stream_key, StreamStatus state_code);

	bool GetStatus(const StreamKey& stream_key, StreamStatus& state_code);

	bool AppendStreamData(const StreamKey& stream_key, std::shared_ptr<FrameInfo>& frame);
	
	int	 GarbageCheck(std::vector<StreamKey>& RemoveStreamKeys);

	int GetStreamCount();

	bool GetPlaylist(const StreamKey& stream_key, PlaylistType type, std::string& playlist);

	bool GetSegmentData(const StreamKey& stream_key,
						const std::string& file_name,
						SegmentType type,
						std::shared_ptr<std::vector<uint8_t>>& data);

private:
	std::shared_ptr<StreamInfo> FindStream(const StreamKey& stream_key);


private:
	std::shared_ptr<MainObject> _main_object;
	std::map<StreamKey, std::shared_ptr<StreamInfo>> _stream_list;
	std::mutex _stream_list_mutex;
};