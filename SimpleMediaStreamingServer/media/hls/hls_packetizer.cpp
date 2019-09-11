﻿//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "hls_packetizer.h"
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <array>
#include <algorithm>

//====================================================================================================
// Constructor
//====================================================================================================
HlsPacketizer::HlsPacketizer(const std::string& app_name,
							const std::string& stream_name,
							PacketizerStreamingType streaming_type,
							const std::string& segment_prefix,
							uint32_t segment_duration,
							uint32_t segment_count,
							MediaInfo& media_info) : Packetizer(app_name,
													stream_name,
													PacketizerType::Hls,
													streaming_type,
													segment_prefix,													
													(uint32_t)segment_duration,
													segment_count, 
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
bool HlsPacketizer::AppendVideoFrame(std::shared_ptr<FrameInfo>& frame)
{
	if (!_video_init)
	{
		// First Key Frame Wait
		if (frame->type != FrameType::VideoKeyFrame)
		{
			return true;
		}

		_video_init = true;
	}

	if (frame->timescale != _media_info.video_timescale)
	{
		frame->timestamp = ConvertTimescale(frame->timestamp, frame->timescale, _media_info.video_timescale);
		frame->cts = ConvertTimescale(frame->cts, frame->timescale, _media_info.video_timescale);
		frame->timescale = _media_info.video_timescale;
	}

	if (frame->type == FrameType::VideoKeyFrame && !_frame_list.empty())
	{
		if ((frame->timestamp - _frame_list[0]->timestamp) >=
			((_segment_duration - _duration_margen) * _media_info.video_timescale))
		{
			// Segment Write
			SegmentWrite(_frame_list[0]->timestamp, frame->timestamp - _frame_list[0]->timestamp);
		}
	}

	_frame_list.push_back(frame);

	_last_video_append_time = time(nullptr);
	_video_enable = true;

	return true;
}

//====================================================================================================
// AppendAudioFrame
//====================================================================================================
bool HlsPacketizer::AppendAudioFrame(std::shared_ptr<FrameInfo>& frame)
{
	if (!_audio_init)
		_audio_init = true;

	if (frame->timescale == _media_info.audio_timescale)
	{
		frame->timestamp = ConvertTimescale(frame->timestamp, frame->timescale, _media_info.video_timescale);
		frame->timescale = _media_info.audio_timescale;
	}

	if ((time(nullptr) - _last_video_append_time >= static_cast<uint32_t>(_segment_duration)) && !_frame_list.empty())
	{
		if ((frame->timestamp - _frame_list[0]->timestamp) >=
			((_segment_duration - _duration_margen) * _media_info.audio_timescale))
		{
			SegmentWrite(_frame_list[0]->timestamp, frame->timestamp - _frame_list[0]->timestamp);
		}
	}

	_frame_list.push_back(frame);

	_last_audio_append_time = time(nullptr);
	_audio_enable = true;

	return true;
}

//====================================================================================================
// Segment Write
//====================================================================================================
bool HlsPacketizer::SegmentWrite(uint64_t start_timestamp, uint64_t duration)
{
	int64_t _first_audio_time_stamp = 0;
	int64_t _first_video_time_stamp = 0;

	auto ts_writer = std::make_unique<TsWriter>(_video_enable, _audio_enable);

	for (auto& frame : _frame_list)
	{
		// TS(PES) Write
		ts_writer->WriteSample(frame->type != FrameType::AudioFrame,
							frame->type == FrameType::AudioFrame || frame->type == FrameType::VideoKeyFrame,
							frame->timestamp,
							frame->cts,
							frame->data);

		if (_first_audio_time_stamp == 0 && frame->type == FrameType::AudioFrame)
		{
			_first_audio_time_stamp = frame->timestamp;
		}			
		else if (_first_video_time_stamp == 0 && frame->type != FrameType::AudioFrame)
		{
			_first_video_time_stamp = frame->timestamp;
		}
	}

	_frame_list.clear();

	if (_first_audio_time_stamp != 0 && _first_video_time_stamp != 0)
		LOG_DEBUG_WRITE(("hls segment video/audio timestamp gap(%lldms)", (_first_video_time_stamp - _first_audio_time_stamp) / 90));

	auto ts_data = ts_writer->GetDataStream();

	SetSegmentData(string_format("%s_%u.ts", _segment_prefix.c_str(), _sequence_number),
				duration,
				start_timestamp,
				ts_data);

	UpdatePlaylist();

	_video_enable = false;
	_audio_enable = false;

	return true;
}

//====================================================================================================
// PlayList(M3U8) update 
//====================================================================================================
bool HlsPacketizer::UpdatePlaylist()
{
	std::ostringstream play_list_stream;
	std::ostringstream m3u8_play_list;
	double max_duration = 0;

	std::vector<std::shared_ptr<SegmentInfo>> segment_list;
	Packetizer::GetVideoPlaySegments(segment_list);

	for (const auto& segment_data : segment_list)
	{
		m3u8_play_list << "#EXTINF:" << std::fixed << std::setprecision(3)
			<< (double)(segment_data->duration) / (double)(HLS_TIMESCALE) << ",\r\n"
			<< segment_data->file_name.c_str() << "\r\n";

		if (segment_data->duration > max_duration)
		{
			max_duration = segment_data->duration;
		}
	}

	play_list_stream << "#EXTM3U" << "\r\n"
		<< "#EXT-X-MEDIA-SEQUENCE:" << (_sequence_number - 1) << "\r\n"
		<< "#EXT-X-VERSION:3" << "\r\n"
		<< "#EXT-X-ALLOW-CACHE:NO" << "\r\n"
		<< "#EXT-X-TARGETDURATION:" << (int)(max_duration / HLS_TIMESCALE) << "\r\n"
		<< m3u8_play_list.str();

	// Playlist 설정
	std::string playlist = play_list_stream.str().c_str();
	SetPlayList(playlist);

	if (_streaming_type == PacketizerStreamingType::Both && _streaming_start)
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
const std::shared_ptr<SegmentInfo> HlsPacketizer::GetSegmentData(const std::string& file_name)
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
bool HlsPacketizer::SetSegmentData(std::string file_name,
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