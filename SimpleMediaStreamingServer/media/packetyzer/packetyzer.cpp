//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "packetyzer.h"
#include <sstream>
#include <algorithm>

//====================================================================================================
// Constructor
//====================================================================================================
Packetyzer::Packetyzer(const std::string& app_name,
	const std::string& stream_name,
	PacketyzerType packetyzer_type,
	PacketyzerStreamingType streaming_type,
	const std::string& segment_prefix,
	uint32_t segment_duration,
	uint32_t segment_count,
	MediaInfo& media_info)
{
	_app_name = app_name;
	_stream_name = stream_name;
	_packetyzer_type = packetyzer_type;
	_streaming_type = streaming_type;
	_segment_prefix = segment_prefix;
	_segment_duration = segment_duration;
	_segment_count = segment_count;
	_segment_save_count = segment_count * 10;	

	_media_info = media_info;
	_sequence_number = 1;

	_video_init = false;
	_audio_init = false;

	_streaming_start = false;

	// init nullptr
	for (uint32_t index = 0; index < _segment_save_count; index++)
	{
		_video_segment_datas.push_back(nullptr);

		// only dash/cmaf
		if (_packetyzer_type == PacketyzerType::Dash || _packetyzer_type == PacketyzerType::Cmaf)
			_audio_segment_datas.push_back(nullptr);
	}
}

//====================================================================================================
// Destructor
//====================================================================================================
Packetyzer::~Packetyzer()
{

}

//====================================================================================================
// MakeUtcSecond(second)
//====================================================================================================
std::string Packetyzer::MakeUtcSecond(time_t value)
{
	std::tm* now_tm = gmtime(&value);
	char buffer[42];

	strftime(buffer, sizeof(buffer), "\"%Y-%m-%dT%TZ\"", now_tm);

	return buffer;
}

//====================================================================================================
// MakeUtcMillisecond(mille second)
//====================================================================================================
std::string Packetyzer::MakeUtcMillisecond(double value)
{
	time_t time_vaule = (time_t)value / 1000;
	std::tm* now_tm = gmtime(&time_vaule);
	char buffer[42];

	strftime(buffer, sizeof(buffer), "%Y-%m-%dT%T", now_tm);

	return string_format("\"%s.%uZ\"", buffer, (uint32_t)value % 1000);
}

//====================================================================================================
// PlayList
// - thread safe
//====================================================================================================
void Packetyzer::SetPlayList(std::string& playlist)
{
	// playlist mutex
	std::unique_lock<std::mutex> lock(_play_list_guard);
	_playlist = playlist;
}

//====================================================================================================
// PlayList
// - thread safe
//====================================================================================================
bool Packetyzer::GetPlayList(std::string& playlist)
{
	if (!_streaming_start)
		return false;

	// playlist mutex
	std::unique_lock<std::mutex> lock(_play_list_guard);

	playlist = _playlist;

	return true;
}

//====================================================================================================
// Last (segment count) Video(or Video+Audio) Segments
// - thread safe
//====================================================================================================
bool Packetyzer::GetVideoPlaySegments(std::vector<std::shared_ptr<SegmentInfo>>& segment_list)
{

	uint32_t begin_index = (_current_video_index >= _segment_count) ?
		(_current_video_index - _segment_count) :
		(_segment_save_count - (_segment_count - _current_video_index));


	uint32_t end_index = (begin_index <= (_segment_save_count - _segment_count)) ?
		(begin_index + _segment_count) - 1 :
		(_segment_count - (_segment_save_count - begin_index)) - 1;

	// video segment mutex
	std::unique_lock<std::mutex> lock(_video_segment_guard);

	if (begin_index <= end_index)
	{
		for (auto item = _video_segment_datas.begin() + begin_index; item <= _video_segment_datas.begin() + end_index; item++)
		{
			if (*item == nullptr)
				return true;

			segment_list.push_back(*item);
		}
	}
	else
	{
		for (auto item = _video_segment_datas.begin() + begin_index; item < _video_segment_datas.end(); item++)
		{
			if (*item == nullptr)
				return true;

			segment_list.push_back(*item);
		}

		for (auto item = _video_segment_datas.begin(); item <= _video_segment_datas.begin() + end_index; item++)
		{
			if (*item == nullptr)
				return true;

			segment_list.push_back(*item);
		}
	}

	return true;
}

//====================================================================================================
// Last (segment count) Audio Segments
// - thread safe
//====================================================================================================
bool Packetyzer::GetAudioPlaySegments(std::vector<std::shared_ptr<SegmentInfo>>& segment_list)
{
	uint32_t begin_index = (_current_audio_index >= _segment_count) ?
		(_current_audio_index - _segment_count) :
		(_segment_save_count - (_segment_count - _current_audio_index));


	uint32_t end_index = (begin_index <= (_segment_save_count - _segment_count)) ?
		(begin_index + _segment_count) - 1 :
		(_segment_count - (_segment_save_count - begin_index)) - 1;

	// audio segment mutex
	std::unique_lock<std::mutex> lock(_audio_segment_guard);

	if (begin_index <= end_index)
	{
		for (auto item = _audio_segment_datas.begin() + begin_index; item <= _audio_segment_datas.begin() + end_index; item++)
		{
			if (*item == nullptr)
				return true;

			segment_list.push_back(*item);
		}
	}
	else
	{
		for (auto item = _audio_segment_datas.begin() + begin_index; item < _audio_segment_datas.end(); item++)
		{
			if (*item == nullptr)
				return true;

			segment_list.push_back(*item);
		}
		for (auto item = _audio_segment_datas.begin(); item <= _audio_segment_datas.begin() + end_index; item++)
		{
			if (*item == nullptr)
				return true;

			segment_list.push_back(*item);
		}
	}

	return true;
}