//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "common/common_function.h"
#include "media/media_define.h"
#include <mutex>
#include <memory>
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

	StreamInfo(const StreamKey& stream_key, int provider_index_key_, uint32_t provider_ip_)
	{
		create_time = time(nullptr);
		state_code = StreamStatus::Ready;
		error_code = StreamError::None;
		provider_index_key = provider_index_key_;
		provider_ip = provider_ip_;

	}

public:
	time_t			create_time;
	StreamStatus	state_code;
	StreamError		error_code;

	int				provider_index_key;
	uint32_t		provider_ip;

	MediaInfo		media_info;

	std::map<int, uint32_t> publisher_indexer;

	// TODO : packetyzer
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
	bool AddPublisher(const StreamKey& stream_key, int publisher_index_key, uint32_t publisher_ip, MediaInfo& media_info);

	bool Remove(const StreamKey& stream_key);
	void RemoveAll();
	bool SetError(const StreamKey& stream_key, StreamError error_code);
	bool SetStatus(const StreamKey& stream_key, StreamStatus state_code);

	bool GetStatus(const StreamKey& stream_key, StreamStatus& state_code);

	bool AppendStreamData(const StreamKey& stream_key,
		const std::shared_ptr<FrameInfo>& frame_info,
		const std::vector<int>& publisher_index_key_list);
	bool RemovePublisherIndexKey(const StreamKey& stream_key, int publisher_index_key);

	int	 GarbageCheck(std::vector<StreamKey>& RemoveStreamKeys);
	void GetCountInfo(int& stream_count);

	bool GetPlaylist(const StreamKey& stream_key, PlaylistType type, std::string& play_list);
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