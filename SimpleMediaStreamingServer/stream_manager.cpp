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

		// temp(hls packetizer create)
		stream_info->CreatePacketizer(media_info, stream_key.second, 5, 3);

		stream_info->state_code = StreamStatus::Complete;

		result = true;
	}

	stream_info_map_lock.unlock();

	return result;
}

//====================================================================================================
// Remove
//====================================================================================================
bool StreamManager::Remove(const StreamKey& stream_key)
{
	int provider_index_key = -1;

	std::unique_lock<std::mutex> stream_info_map_lock(_stream_list_mutex);

	auto item = _stream_list.find(stream_key);

	if (item != _stream_list.end())
	{
		auto stream_info = item->second;

		if (stream_info != nullptr)
		{
			provider_index_key = stream_info->provider_index_key;

		}
		_stream_list.erase(item);
	}

	stream_info_map_lock.unlock();

	if (provider_index_key != -1)
	{
		_main_object->RemoveNetwork(NetworkObjectKey::RtmpEncoder, provider_index_key);
	}

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
bool StreamManager::GetPlaylist(const StreamKey& stream_key, PlaylistType type, std::string& playlist)
{
	auto stream_info = FindStream(stream_key);

	if (stream_info != nullptr)
	{
		return stream_info->hls_packetizer->GetPlayList(playlist);
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
		auto segment = stream_info->hls_packetizer->GetSegmentData(file_name);
		if (segment != nullptr)
		{
			data = segment->data;
			return true;
		}
	}

	return false;
}

//====================================================================================================
// Stream Data Input
//====================================================================================================
bool StreamManager::AppendStreamData(const StreamKey& stream_key, std::shared_ptr<FrameInfo>& frame)
{
	auto stream_info = FindStream(stream_key);
	
	if (stream_info != nullptr)
	{
		stream_info->AppendFrame(frame);
		return true;
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
				LOG_ERROR_WRITE(("*** Stream Error Remove - stream(%s/%s) status(%s) error(%s)***",
					item->first.first.c_str(),
					item->first.second.c_str(),
					g_stream_status_string[(int)stream_info->state_code],
					g_stream_error_string[(int)stream_info->error_code]));

				remove = true;
			}
			// Max read wait check
			else if (stream_info->state_code == StreamStatus::Ready && (create_time_gap > READY_STATUS_MAX_WAIT_TIME))
			{
				LOG_ERROR_WRITE(("*** Stream Ready Time Over Remove - stream(%s/%s) status(%s) gap(%d:%d) ***",
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
				LOG_ERROR_WRITE(("*** Stream Create Time Over Remove - stream(%s/%s) Status(%s) gap(%d:%d) ***",
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
	if (provider_index_key_list.size() > 0)
	{
		_main_object->RemoveNetwork(NetworkObjectKey::RtmpEncoder, provider_index_key_list);
	}

	return remove_count;
}

//====================================================================================================
// Count info
//====================================================================================================
int StreamManager::GetStreamCount()
{
	std::unique_lock<std::mutex> stream_info_map_lock(_stream_list_mutex);

	return (int)_stream_list.size();
}