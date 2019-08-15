//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "stream_manager.h"
#include "main_object.h" 

#define READY_STATUS_MAX_WAIT_TIME (5)
#define COMPLETE_STATUS_MAX_WAIT_TIME (10)
//====================================================================================================
// Constructor 
//====================================================================================================
StreamManager::StreamManager(std::shared_ptr<MainObject>& main_object)
{
	_main_object = main_object;
}

//====================================================================================================
// Destructor 
//====================================================================================================
StreamManager::~StreamManager()
{
	RemoveAll();
}

//====================================================================================================
// CreateStream 
//====================================================================================================
bool StreamManager::CreateStream(const StreamKey& stream_key, int provider_index_key, uint32_t provider_ip)
{
	bool result = false;

	std::unique_lock<std::mutex> stream_info_map_lock(_stream_list_mutex);

	auto item = _stream_list.find(stream_key);

	// add stream
	if (item == _stream_list.end())
	{
		auto stream_info = std::make_shared<StreamInfo>(stream_key, provider_index_key, provider_ip);

		_stream_list[stream_key] = stream_info;
		result = true;
	}

	return result;
}

//====================================================================================================
// SetCompleteState
//====================================================================================================
bool StreamManager::SetCompleteState(const StreamKey& stream_key, MediaInfo& media_info)
{
	bool result = false;

	std::unique_lock<std::mutex> stream_info_map_lock(_stream_list_mutex);

	auto item = _stream_list.find(stream_key);

	if (item != _stream_list.end())
	{
		auto stream_info = item->second;


		stream_info->media_info = media_info;
		stream_info->state_code = StreamStatus::Complete;

		/*
		PacketyzerMediaInfo packtyzer_media_info(SegmentCodecType::H264Codec,
			media_info.video_width,
			media_info.video_height,
			media_info.video_framerate,
			media_info.video_bitrate,
			media_info.timestamp_scale,
			SegmentCodecType::AacCodec,
			media_info.audio_channels,
			media_info.audio_samplerate,
			media_info.audio_bitrate,
			media_info.timestamp_scale);

		stream_info->stream_packetyzer->Create(stream_key.second,
			PacketyzerStreamType::CommonStrem,
			5,
			10,
			media_info.avc_sps,
			media_info.avc_pps,
			media_info.audio_sampleindex,
			packtyzer_media_info);

*/
		result = true;
	}

	stream_info_map_lock.unlock();

	return result;
}


//====================================================================================================
// Add Publisher
//====================================================================================================
bool StreamManager::AddPublisher(const StreamKey& stream_key, int publisher_index_key, uint32_t publisher_ip, MediaInfo& media_info)
{
	bool result = false;

	std::unique_lock<std::mutex> stream_info_map_lock(_stream_list_mutex);

	auto item = _stream_list.find(stream_key);
	if (item != _stream_list.end())
	{
		auto stream_info = item->second;

		if (stream_info->state_code == StreamStatus::Complete)
		{
			stream_info->publisher_indexer[publisher_index_key] = publisher_ip;
			media_info = stream_info->media_info;

			result = true;
		}
	}

	return result;

}



//====================================================================================================
// Remove
//====================================================================================================
bool StreamManager::Remove(const StreamKey& stream_key)
{
	int provider_index_key = -1;
	std::vector<int> publisher_index_key_list;

	std::unique_lock<std::mutex> stream_info_map_lock(_stream_list_mutex);

	auto item = _stream_list.find(stream_key);

	if (item != _stream_list.end())
	{
		auto stream_info = item->second;

		if (stream_info != nullptr)
		{
			provider_index_key = stream_info->provider_index_key;

			for (auto IndexerItem = stream_info->publisher_indexer.begin(); IndexerItem != stream_info->publisher_indexer.end(); IndexerItem++)
			{
				publisher_index_key_list.push_back(IndexerItem->first);
			}
			stream_info->publisher_indexer.clear();
		}
		_stream_list.erase(item);
	}

	stream_info_map_lock.unlock();

	if (provider_index_key != -1) _main_object->RemoveNetwork(NetworkObjectKey::RtmpEncoder, provider_index_key);
	if (publisher_index_key_list.empty() == false) _main_object->RemoveNetwork(NetworkObjectKey::HttpClient, publisher_index_key_list);

	return true;

}

//====================================================================================================
// Remove All
//====================================================================================================
void StreamManager::RemoveAll()
{
	std::unique_lock<std::mutex> stream_info_map_lock(_stream_list_mutex);
	_stream_list.clear();
}

//====================================================================================================
// FindStream
//====================================================================================================
std::shared_ptr<StreamInfo> StreamManager::FindStream(const StreamKey& stream_key)
{
	std::unique_lock<std::mutex> stream_info_map_lock(_stream_list_mutex);

	auto item = _stream_list.find(stream_key);

	if (item != _stream_list.end())
	{
		return item->second;
	}

	return nullptr;
}

//====================================================================================================
// Error code Setting 
//====================================================================================================
bool StreamManager::SetError(const StreamKey& stream_key, StreamError error_code)
{
	auto stream_info = FindStream(stream_key);

	if (stream_info != nullptr)
	{
		stream_info->error_code = error_code;
		return true;
	}

	return false;
}

//====================================================================================================
// Status code setting
//====================================================================================================
bool StreamManager::SetStatus(const StreamKey& stream_key, StreamStatus state_code)
{
	auto stream_info = FindStream(stream_key);

	if (stream_info != nullptr)
	{
		stream_info->state_code = state_code;
		return true;
	}

	return false;
}




//====================================================================================================
// Status code
//====================================================================================================
bool StreamManager::GetStatus(const StreamKey& stream_key, StreamStatus& state_code)
{
	auto stream_info = FindStream(stream_key);

	if (stream_info != nullptr)
	{
		state_code = stream_info->state_code;
		return true;
	}

	return false;
}

//====================================================================================================
// GetPlayList
//====================================================================================================
bool StreamManager::GetPlaylist(const StreamKey& stream_key, PlaylistType type, std::string& play_list)
{
	auto stream_info = FindStream(stream_key);

	if (stream_info != nullptr)
	{
		// return stream_info->stream_packetyzer->GetPlayList(type, play_list);
	}

	return false;
}

//====================================================================================================
// GetSegment
//====================================================================================================
bool StreamManager::GetSegmentData(const StreamKey& stream_key,
	const std::string& file_name,
	SegmentType type,
	std::shared_ptr<std::vector<uint8_t>>& data)
{
	auto stream_info = FindStream(stream_key);

	if (stream_info != nullptr)
	{
		// return stream_info->stream_packetyzer->GetSegment(file_name, type, data);
	}

	return false;
}

//====================================================================================================
// Stream Data Input
//====================================================================================================
bool StreamManager::AppendStreamData(const StreamKey& stream_key,
	const std::shared_ptr<FrameInfo>& frame_info,
	const std::vector<int>& publisher_index_key_list)
{
	auto stream_info = FindStream(stream_key);

	/*
	if (stream_info != nullptr)
	{
		// - Packetyzer 삽입
		if (frame_info->frame_type != FrameType::AudioFrame)
			stream_info->stream_packetyzer->AppendVideoData(frame_info->timestamp, RTMP_TIMESCALE, frame_info->frame_type == FrameType::VideoKeyFrame, frame_info->cts, frame_info->frame_data->size(), frame_info->frame_data->data());
		else
			stream_info->stream_packetyzer->AppendAudioData(frame_info->timestamp, RTMP_TIMESCALE, frame_info->frame_data->size(), frame_info->frame_data->data());

		for (auto IndexerItem = stream_info->publisher_indexer.begin(); IndexerItem != stream_info->publisher_indexer.end(); IndexerItem++)
		{
			publisher_index_key_list.push_back(IndexerItem->first);
		}

		return true;
	}
*/
	return false;
}

//====================================================================================================
// Client Index
//====================================================================================================
bool StreamManager::RemovePublisherIndexKey(const StreamKey& stream_key, int publisher_index_key)
{
	auto stream_info = FindStream(stream_key);

	if (stream_info != nullptr)
	{
		auto IndexerItem = stream_info->publisher_indexer.find(publisher_index_key);

		if (IndexerItem != stream_info->publisher_indexer.end())
		{
			stream_info->publisher_indexer.erase(IndexerItem);
			return true;
		}
	}

	return false;
}

//====================================================================================================
// Garbage Check
// - Error code check
// - Stream keepalive check
//====================================================================================================
int StreamManager::GarbageCheck(std::vector<StreamKey>& remove_stream_keys)
{
	time_t	current_time = time(nullptr);
	int		create_time_gap = 0;
	bool	remove = false;
	int 	remove_count = 0;

	std::vector<int> provider_index_key_list;
	std::vector<int> publisher_index_key_list;

	std::unique_lock<std::mutex> stream_info_map_lock(_stream_list_mutex);

	for (auto item = _stream_list.begin(); item != _stream_list.end();)
	{
		remove = false;

		auto stream_info = item->second;

		create_time_gap = current_time - stream_info->create_time;

		if (stream_info != nullptr)
		{
			// Error code check
			if (stream_info->error_code != StreamError::None)
			{
				LOG_ERROR_WRITE(("*** Stream Error Remove - Stream(%s/%s) Status(%s) Error(%s)***",
					item->first.first.c_str(),
					item->first.second.c_str(),
					g_stream_status_string[(int)stream_info->state_code],
					g_stream_error_string[(int)stream_info->error_code]));

				remove = true;
			}
			// Max read wait check
			else if (stream_info->state_code == StreamStatus::Ready && (create_time_gap > READY_STATUS_MAX_WAIT_TIME))
			{
				LOG_ERROR_WRITE(("*** Stream Ready Time Over Remove - Stream(%s/%s) Status(%s) Gap(%d:%d) ***",
					item->first.first.c_str(),
					item->first.second.c_str(),
					g_stream_status_string[(int)stream_info->state_code],
					create_time_gap,
					READY_STATUS_MAX_WAIT_TIME));

				remove = true;
			}
			// Max complete wait check
			else if (stream_info->state_code != StreamStatus::Complete && (create_time_gap > COMPLETE_STATUS_MAX_WAIT_TIME))
			{
				LOG_ERROR_WRITE(("*** Stream Create Time Over Remove - Stream(%s/%s) Status(%s) Gap(%d:%d) ***",
					item->first.first.c_str(),
					item->first.second.c_str(),
					g_stream_status_string[(int)stream_info->state_code],
					create_time_gap,
					COMPLETE_STATUS_MAX_WAIT_TIME));

				remove = true;
			}

		}

		// remove
		if (remove == true)
		{
			remove_stream_keys.push_back(item->first);

			provider_index_key_list.push_back(stream_info->provider_index_key);

			for (auto IndexerItem = stream_info->publisher_indexer.begin(); IndexerItem != stream_info->publisher_indexer.end(); IndexerItem++)
			{
				publisher_index_key_list.push_back(IndexerItem->first);
			}

			remove_count++;

			_stream_list.erase(item++);
		}
		else
		{
			item++;
		}
	}

	stream_info_map_lock.unlock();

	// Session close
	if (provider_index_key_list.size() > 0)		_main_object->RemoveNetwork(NetworkObjectKey::RtmpEncoder, provider_index_key_list);
	if (publisher_index_key_list.size() > 0)	_main_object->RemoveNetwork(NetworkObjectKey::HttpClient, publisher_index_key_list);

	return remove_count;
}

//====================================================================================================
// Count info
//====================================================================================================
void StreamManager::GetCountInfo(int& stream_count)
{
	std::unique_lock<std::mutex> stream_info_map_lock(_stream_list_mutex);

	stream_count = (int)_stream_list.size();
}