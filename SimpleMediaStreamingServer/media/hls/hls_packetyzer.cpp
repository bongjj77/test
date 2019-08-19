﻿//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "hls_packetyzer.h"
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <array>
#include <algorithm>

//====================================================================================================
// Constructor
//====================================================================================================
HlsPacketyzer::HlsPacketyzer(const std::string& app_name,
	const std::string& stream_name,
	PacketyzerStreamingType streaming_type,
	const std::string& segment_prefix,
	uint32_t segment_count,
	uint32_t segment_duration,
	MediaInfo& media_info) :
	Packetyzer(app_name,
		stream_name,
		PacketyzerType::Hls,
		streaming_type,
		segment_prefix,
		segment_count,
		(uint32_t)segment_duration,
		media_info)
{
	_last_video_append_time = time(nullptr);
	_last_audio_append_time = time(nullptr);
	_video_enable = false;
	_audio_enable = false;
	_duration_margen = _segment_duration * 0.1;
}

//====================================================================================================
// AppendVideoFrame
//====================================================================================================
bool HlsPacketyzer::AppendVideoFrame(std::shared_ptr<FrameInfo>& frame_data)
{
	if (!_video_init)
	{
		// First Key Frame Wait
		if (frame_data->frame_type != FrameType::VideoKeyFrame)
		{
			return true;
		}

		_video_init = true;
	}

	if (frame_data->timescale != _media_info.video_timescale)
	{
		frame_data->timestamp = ConvertTimescale(frame_data->timestamp, frame_data->timescale, _media_info.video_timescale);
		frame_data->cts = ConvertTimescale(frame_data->cts, frame_data->timescale, _media_info.video_timescale);
		frame_data->timescale = _media_info.video_timescale;
	}

	if (frame_data->frame_type == FrameType::VideoKeyFrame && !_frame_datas.empty())
	{
		if ((frame_data->timestamp - _frame_datas[0]->timestamp) >=
			((_segment_duration - _duration_margen) * _media_info.video_timescale))
		{
			// Segment Write
			SegmentWrite(_frame_datas[0]->timestamp, frame_data->timestamp - _frame_datas[0]->timestamp);
		}
	}

	_frame_datas.push_back(frame_data);

	_last_video_append_time = time(nullptr);
	_video_enable = true;

	return true;

}

//====================================================================================================
// AppendAudioFrame
//====================================================================================================
bool HlsPacketyzer::AppendAudioFrame(std::shared_ptr<FrameInfo>& frame_data)
{
	if (!_audio_init)
		_audio_init = true;

	if (frame_data->timescale == _media_info.audio_timescale)
	{
		frame_data->timestamp = ConvertTimescale(frame_data->timestamp, frame_data->timescale, _media_info.video_timescale);
		frame_data->timescale = _media_info.audio_timescale;
	}

	if ((time(nullptr) - _last_video_append_time >= static_cast<uint32_t>(_segment_duration)) && !_frame_datas.empty())
	{
		if ((frame_data->timestamp - _frame_datas[0]->timestamp) >=
			((_segment_duration - _duration_margen) * _media_info.audio_timescale))
		{
			SegmentWrite(_frame_datas[0]->timestamp, frame_data->timestamp - _frame_datas[0]->timestamp);
		}
	}

	_frame_datas.push_back(frame_data);

	_last_audio_append_time = time(nullptr);
	_audio_enable = true;

	return true;
}

//====================================================================================================
// Segment Write
//====================================================================================================
bool HlsPacketyzer::SegmentWrite(uint64_t start_timestamp, uint64_t duration)
{
	int64_t _first_audio_time_stamp = 0;
	int64_t _first_video_time_stamp = 0;

	auto ts_writer = std::make_unique<TsWriter>(_video_enable, _audio_enable);

	for (auto& frame_data : _frame_datas)
	{
		// TS(PES) Write
		ts_writer->WriteSample(frame_data->frame_type != FrameType::AudioFrame,
							frame_data->frame_type == FrameType::AudioFrame || frame_data->frame_type == FrameType::VideoKeyFrame,
							frame_data->timestamp,
							frame_data->cts,
							frame_data->frame_data);

		if (_first_audio_time_stamp == 0 && frame_data->frame_type == FrameType::AudioFrame)
		{
			_first_audio_time_stamp = frame_data->timestamp;
		}			
		else if (_first_video_time_stamp == 0 && frame_data->frame_type != FrameType::AudioFrame)
		{
			_first_video_time_stamp = frame_data->timestamp;
		}
	}

	_frame_datas.clear();

	if (_first_audio_time_stamp != 0 && _first_video_time_stamp != 0)
		LOG_DEBUG_WRITE(("hls segment video/audio timestamp gap(%lldms)", (_first_video_time_stamp - _first_audio_time_stamp) / 90));

	auto ts_data = ts_writer->GetDataStream();

	SetSegmentData(string_format("%s_%u.ts", _segment_prefix.c_str(), _sequence_number),
				duration,
				start_timestamp,
				ts_data);

	UpdatePlayList();

	_video_enable = false;
	_audio_enable = false;

	return true;
}

//====================================================================================================
// PlayList(M3U8) update 
//====================================================================================================
bool HlsPacketyzer::UpdatePlayList()
{
	std::ostringstream play_lis_stream;
	std::ostringstream m3u8_play_list;
	double max_duration = 0;

	std::vector<std::shared_ptr<SegmentInfo>> segment_datas;
	Packetyzer::GetVideoPlaySegments(segment_datas);

	for (const auto& segment_data : segment_datas)
	{
		m3u8_play_list << "#EXTINF:" << std::fixed << std::setprecision(3)
			<< (double)(segment_data->duration) / (double)(HLS_TIMESCALE) << ",\r\n"
			<< segment_data->file_name.c_str() << "\r\n";

		if (segment_data->duration > max_duration)
		{
			max_duration = segment_data->duration;
		}
	}

	play_lis_stream << "#EXTM3U" << "\r\n"
		<< "#EXT-X-MEDIA-SEQUENCE:" << (_sequence_number - 1) << "\r\n"
		<< "#EXT-X-VERSION:3" << "\r\n"
		<< "#EXT-X-ALLOW-CACHE:NO" << "\r\n"
		<< "#EXT-X-TARGETDURATION:" << (int)(max_duration / HLS_TIMESCALE) << "\r\n"
		<< m3u8_play_list.str();

	// Playlist 설정
	std::string play_list = play_lis_stream.str().c_str();
	SetPlayList(play_list);

	if (_streaming_type == PacketyzerStreamingType::Both && _streaming_start)
	{
		if (!_video_enable)
			LOG_WARNING_WRITE(("Hls video segment problems - %s/%s", _app_name.c_str(), _stream_name.c_str()));

		if (!_audio_enable)
			LOG_WARNING_WRITE(("Hls audio segment problems - %s/%s", _app_name.c_str(), _stream_name.c_str()));
	}

	return true;
}

//====================================================================================================
// Get Segment
//====================================================================================================
const std::shared_ptr<SegmentInfo> HlsPacketyzer::GetSegmentData(const std::string& file_name)
{
	if (!_streaming_start)
		return nullptr;

	// video segment mutex
	std::unique_lock<std::mutex> lock(_video_segment_guard);

	auto item = std::find_if(_video_segment_datas.begin(), _video_segment_datas.end(), [&](std::shared_ptr<SegmentInfo> const& value) -> bool
							{
								return value != nullptr ? value->file_name == file_name : false;
							});

	if (item == _video_segment_datas.end())
	{
		return nullptr;
	}

	return (*item);
}

//====================================================================================================
// Set Segment
//====================================================================================================
bool HlsPacketyzer::SetSegmentData(std::string file_name,
	uint64_t duration,
	uint64_t timestamp,
	std::shared_ptr<std::vector<uint8_t>>& data)
{
	auto segment_data = std::make_shared<SegmentInfo>(_sequence_number++,
		file_name,
		duration,
		timestamp,
		data);

	// video segment mutex
	std::unique_lock<std::mutex> lock(_video_segment_guard);

	_video_segment_datas[_current_video_index++] = segment_data;

	if (_segment_save_count <= _current_video_index)
		_current_video_index = 0;

	if (!_streaming_start && _sequence_number > _segment_count)
	{
		_streaming_start = true;
		LOG_INFO_WRITE(("Hls segment ready completed - stream(%s/%s) segment(%ds/%d)",
			_app_name.c_str(), _stream_name.c_str(), _segment_duration, _segment_count));
	}

	return true;
}
