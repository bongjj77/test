//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "dash_packetizer.h"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <numeric>

#define VIDEO_TRACK_ID    (1)
#define AUDIO_TRACK_ID    (2)

//====================================================================================================
// Constructor
//====================================================================================================
DashPacketizer::DashPacketizer(const std::string& app_name,
							const std::string& stream_name,
							PacketizerStreamingType stream_type,
							const std::string& segment_prefix,
							uint32_t segment_duration,
							uint32_t segment_count,
							MediaInfo& media_info) : Packetizer(app_name,
																stream_name,
																PacketizerType::Dash,
																stream_type,
																segment_prefix,
																segment_duration,
																segment_count,
																media_info)
{
	_video_frame_list.clear();
	_audio_frame_list.clear();

	uint32_t resolution_gcd = Gcd(media_info.video_width, media_info.video_height);

	if (resolution_gcd != 0)
	{
		std::ostringstream pixel_aspect_ratio;
		pixel_aspect_ratio << media_info.video_width / resolution_gcd << ":"
			<< media_info.video_height / resolution_gcd;

		_mpd_pixel_aspect_ratio = pixel_aspect_ratio.str();
	}

	_mpd_suggested_presentation_delay = _segment_duration; 

	_mpd_min_buffer_time = 6;

	_last_video_append_time = time(nullptr);

	_last_audio_append_time = time(nullptr);

	_duration_margen = _segment_duration * 0.1;
}

//====================================================================================================
// Destructor
//====================================================================================================
DashPacketizer::~DashPacketizer()
{
	_video_frame_list.clear();
	_audio_frame_list.clear();
}

//====================================================================================================
// Dash File Type
//====================================================================================================
DashFileType DashPacketizer::GetFileType(const std::string& file_name)
{
	if (file_name == DASH_MPD_VIDEO_INIT_FILE_NAME)
		return DashFileType::VideoInit;
	else if (file_name == DASH_MPD_AUDIO_INIT_FILE_NAME)
		return DashFileType::AudioInit;
	else if (file_name.find(DASH_MPD_VIDEO_SUFFIX) >= 0)
		return DashFileType::VideoSegment;
	else if (file_name.find(DASH_MPD_AUDIO_SUFFIX) >= 0)
		return DashFileType::AudioSegment;

	return DashFileType::Unknown;
}

//====================================================================================================
// Video 설정 값 Load ( Key Frame 데이터만 가능)
//  0001 + sps + 0001 + pps + 0001 + I-Frame 구조 파싱
// - profile
// - level
// - sps/pps
// - init m4s create
//====================================================================================================
bool DashPacketizer::VideoInit(const std::shared_ptr<std::vector<uint8_t>>& frame_data)
{
	
	// Video init m4s Create
	auto writer = std::make_unique<M4sInitWriter>(M4sMediaType::Video,
												_segment_duration * _media_info.video_timescale,
												_media_info.video_timescale,
												VIDEO_TRACK_ID,
												_media_info.video_width,
												_media_info.video_height,
												_media_info.avc_sps,
												_media_info.avc_pps,
												_media_info.audio_channels,
												16,
												_media_info.audio_samplerate);

	auto init_data = writer->CreateData(false);

	if (init_data == nullptr)
	{
		LOG_ERROR_WRITE(("Dash video init writer create fail - stream(%s/%s)", _app_name.c_str(), _stream_name.c_str()));
		return false;
	}
	
	// Video init m4s Save
	_video_init_file = std::make_shared<SegmentInfo>(0, DASH_MPD_VIDEO_INIT_FILE_NAME, 0, 0, init_data);

	return true;
}

//====================================================================================================
// Audio 설정 값 Load
// - sample rate
// - channels
// - init m4s create
//====================================================================================================
bool DashPacketizer::AudioInit()
{
	std::shared_ptr<std::vector<uint8_t>> temp = nullptr;

	// Audio init m4s 생성(메모리)
	auto writer = std::make_unique<M4sInitWriter>(M4sMediaType::Audio,
												_segment_duration * _media_info.audio_timescale,
												_media_info.audio_timescale,
												AUDIO_TRACK_ID,
												_media_info.video_width,
												_media_info.video_height,
												temp,
												temp,
												_media_info.audio_channels,
												16,
												_media_info.audio_samplerate);

	auto init_data = writer->CreateData(false);

	if (init_data == nullptr)
	{
		LOG_ERROR_WRITE(("Dash audio init writer create fail - stream(%s/%s)", _app_name.c_str(), _stream_name.c_str()));
		return false;
	}

	// Audio init m4s Save
	_audio_init_file = std::make_shared<SegmentInfo>(0, DASH_MPD_AUDIO_INIT_FILE_NAME, 0, 0, init_data);

	return true;
}

//====================================================================================================
// Video Frame Append
// - Video(Audio) Segment m4s Create
//====================================================================================================
bool DashPacketizer::AppendVideoFrame(std::shared_ptr<FrameInfo>& frame)
{
	if (!_video_init)
	{
		if (frame->type != FrameType::VideoKeyFrame)
			return false;

		if (!VideoInit(frame->data))
			return false;

		_video_init = true;
	}

	// Fragment Check
	// - KeyFrame ~ KeyFrame(before)
	if (frame->type == FrameType::VideoKeyFrame && !_video_frame_list.empty())
	{
		if (frame->timestamp - _video_frame_list[0]->timestamp >=
			((_segment_duration - _duration_margen) * _media_info.video_timescale))
		{

			VideoSegmentWrite(frame->timestamp);

			if (!_audio_frame_list.empty())
			{
				AudioSegmentWrite(ConvertTimescale(frame->timestamp,
					_media_info.video_timescale,
					_media_info.audio_timescale));
			}

			UpdatePlaylist();
		}
	}

	if (_start_time.empty())
	{
		_start_time = MakeUtcSecond(time(nullptr));
	}

	_video_frame_list.push_back(frame);
	_last_video_append_time = time(nullptr);

	return true;
}

//====================================================================================================
// Audio Frame Append
// - Audio Segment m4s Create
//====================================================================================================
bool DashPacketizer::AppendAudioFrame(std::shared_ptr<FrameInfo>& frame)
{
	if (!_audio_init)
	{
		if (!AudioInit())
			return false;

		_audio_init = true;
	}

	if (_start_time.empty())
	{
		_start_time = MakeUtcSecond(time(nullptr));
	}

	_audio_frame_list.push_back(frame);
	_last_audio_append_time = time(nullptr);

	// audio only or video input error
	if ((time(nullptr) - _last_video_append_time >=
		static_cast<uint32_t>(_segment_duration)) && !_audio_frame_list.empty())
	{
		if ((frame->timestamp - _audio_frame_list[0]->timestamp) >=
			((_segment_duration - _duration_margen) * _media_info.audio_timescale))
		{
			AudioSegmentWrite(frame->timestamp);

			UpdatePlaylist();
		}
	}

	return true;
}

//====================================================================================================
// Video Segment Write
// - Duration/Key Frame 확인 이후 이전 데이터 까지 생성
//====================================================================================================
bool DashPacketizer::VideoSegmentWrite(uint64_t max_timestamp)
{
	uint64_t start_timestamp = 0;
	std::vector<std::shared_ptr<SampleData>> sample_data_list;

	while (!_video_frame_list.empty())
	{
		auto frame = _video_frame_list.front();

		if (start_timestamp == 0)
			start_timestamp = frame->timestamp;

		if (frame->timestamp >= max_timestamp)
			break;

		_video_frame_list.pop_front();

		uint64_t duration = _video_frame_list.empty() ? (max_timestamp - frame->timestamp) :
			(_video_frame_list.front()->timestamp - frame->timestamp);

		auto sample_data = std::make_shared<SampleData>(duration,
			frame->type == FrameType::VideoKeyFrame
			? 0X02000000 : 0X01010000,
			frame->timestamp,
			frame->cts,
			frame->data);

		sample_data_list.push_back(sample_data);
	}
	_video_frame_list.clear();

	// Fragment write
	auto fragment_writer = std::make_unique<M4sSegmentWriter>(M4sMediaType::Video,
															_video_sequence_number,
															VIDEO_TRACK_ID,
															start_timestamp);

	auto segment_data = fragment_writer->AppendSamples(sample_data_list);

	if (segment_data == nullptr)
	{
		LOG_ERROR_WRITE(("Dash video writer create fail - stream(%s/%s)", _app_name.c_str(), _stream_name.c_str()));
		return false;
	}

	// m4s data save
	SetSegmentData(string_format("%s_%lld%s", _segment_prefix.c_str(), start_timestamp, DASH_MPD_VIDEO_SUFFIX),
		max_timestamp - start_timestamp,
		start_timestamp,
		segment_data);

	return true;
}

//====================================================================================================
// Audio Segment Write
// - 비디오 Segment 생성이후 생성 or Audio Only 에서 생성
//====================================================================================================
#define AAC_SAMPLES_PER_FRAME (1024)
bool DashPacketizer::AudioSegmentWrite(uint64_t max_timestamp)
{
	uint64_t start_timestamp = 0;
	uint64_t end_timestamp = 0;
	std::vector<std::shared_ptr<SampleData>> sample_data_list;

	//  size > 1  for duration calculation
	while (_audio_frame_list.size() > 1)
	{
		auto frame = _audio_frame_list.front();

		if (start_timestamp == 0)
			start_timestamp = frame->timestamp;

		if (((frame->timestamp + AAC_SAMPLES_PER_FRAME) > max_timestamp))
			break;

		_audio_frame_list.pop_front();

		auto sample_data = std::make_shared<SampleData>(_audio_frame_list.front()->timestamp - frame->timestamp,
			frame->timestamp,
			frame->data);

		end_timestamp = _audio_frame_list.front()->timestamp;

		sample_data_list.push_back(sample_data);
	}

	// Fragment write
	auto fragment_writer = std::make_unique<M4sSegmentWriter>(M4sMediaType::Audio,
															_audio_sequence_number,
															AUDIO_TRACK_ID,
															start_timestamp);

	auto segment_data = fragment_writer->AppendSamples(sample_data_list);

	if (segment_data == nullptr)
	{
		LOG_ERROR_WRITE(("Dash audio writer create fail - stream(%s/%s)", _app_name.c_str(), _stream_name.c_str()));
		return false;
	}

	// m4s save
	SetSegmentData(string_format("%s_%lld%s", _segment_prefix.c_str(), start_timestamp, DASH_MPD_AUDIO_SUFFIX),
					end_timestamp - start_timestamp,
					start_timestamp,
					segment_data);

	return true;
}

//====================================================================================================
// GetSegmentPlayInfos
// - video/audio mpd timeline
//====================================================================================================
bool DashPacketizer::GetSegmentPlayInfos(std::string& video_urls,
										std::string& audio_urls,
										double& time_shift_buffer_depth,
										double& minimumUpdatePeriod)
{
	std::ostringstream video_urls_stream;
	std::ostringstream audio_urls_stream;
	uint64_t video_total_duration = 0;
	uint64_t video_last_duration = 0;
	uint64_t audio_total_duration = 0;
	uint64_t audio_last_duration = 0;
	std::vector<std::shared_ptr<SegmentInfo>> video_segment_datas;
	std::vector<std::shared_ptr<SegmentInfo>> audio_segment_datas;

	Packetizer::GetVideoPlaySegments(video_segment_datas);
	Packetizer::GetAudioPlaySegments(audio_segment_datas);

	for (uint32_t index = 0; index < video_segment_datas.size(); index++)
	{
		// Timeline Setting
		if (index == 0)
			video_urls_stream << "\t\t\t\t" << "<S t=\"" << video_segment_datas[index]->timestamp
			<< "\" d=\"" << video_segment_datas[index]->duration
			<< "\"/>\n";
		else
			video_urls_stream << "\t\t\t\t" << "<S d=\"" << video_segment_datas[index]->duration << "\"/>\n";

		// total duration
		video_total_duration += video_segment_datas[index]->duration;

		video_last_duration = video_segment_datas[index]->duration;
	}

	video_urls = video_urls_stream.str().c_str();

	for (uint32_t index = 0; index < audio_segment_datas.size(); index++)
	{
		// Timeline Setting
		if (index == 0)
			audio_urls_stream << "\t\t\t\t<S t=\"" << audio_segment_datas[index]->timestamp
			<< "\" d=\"" << audio_segment_datas[index]->duration
			<< "\"/>\n";
		else
			audio_urls_stream << "\t\t\t\t<S d=\"" << audio_segment_datas[index]->duration << "\"/>\n";

		// total duration
		audio_total_duration += audio_segment_datas[index]->duration;

		audio_last_duration = audio_segment_datas[index]->duration;
	}

	audio_urls = audio_urls_stream.str().c_str();

	if (!video_urls.empty() && _media_info.video_timescale != 0)
	{
		time_shift_buffer_depth = video_total_duration / (double)_media_info.video_timescale;
		minimumUpdatePeriod = video_last_duration / (double)_media_info.video_timescale;
	}
	else if (!audio_urls.empty() && _media_info.audio_timescale != 0)
	{
		time_shift_buffer_depth = audio_total_duration / (double)_media_info.audio_timescale;
		minimumUpdatePeriod = audio_last_duration / (double)_media_info.audio_timescale;
	}

	return true;
}

//====================================================================================================
// PlayList(mpd) Update
// - 차후 자동 인덱싱 사용시 참고 : LSN = floor(now - (availabilityStartTime + PST) / segmentDuration + startNumber - 1)
//====================================================================================================
bool DashPacketizer::UpdatePlaylist()
{
	std::ostringstream play_list_stream;
	std::string video_urls;
	std::string audio_urls;
	double time_shift_buffer_depth = 0;
	double minimumUpdatePeriod = 0;

	GetSegmentPlayInfos(video_urls, audio_urls, time_shift_buffer_depth, minimumUpdatePeriod);

	play_list_stream << std::fixed << std::setprecision(3)
		<< "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		<< "<MPD xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
		<< "    xmlns=\"urn:mpeg:dash:schema:mpd:2011\"\n"
		<< "    xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
		<< "    xsi:schemaLocation=\"urn:mpeg:DASH:schema:MPD:2011 http://standards.iso.org/ittf/PubliclyAvailableStandards/MPEG-DASH_schema_files/DASH-MPD.xsd\"\n"
		<< "    profiles=\"urn:mpeg:dash:profile:isoff-live:2011\"\n"
		<< "    type=\"dynamic\"\n"
		<< "    minimumUpdatePeriod=\"PT" << minimumUpdatePeriod << "S\"\n"
		<< "    publishTime=" << MakeUtcSecond(time(nullptr)).c_str() << "\n"
		<< "    availabilityStartTime=" << _start_time.c_str() << "\n"
		<< "    timeShiftBufferDepth=\"PT" << time_shift_buffer_depth << "S\"\n"
		<< "    suggestedPresentationDelay=\"PT" << std::setprecision(1) << _mpd_suggested_presentation_delay << "S\"\n"
		<< "    minBufferTime=\"PT" << _mpd_min_buffer_time << "S\">\n"
		<< "<Period id=\"0\" start=\"PT0S\">\n";

	// video listing
	if (!video_urls.empty())
	{
		play_list_stream << "\t<AdaptationSet id=\"0\" group=\"1\" mimeType=\"video/mp4\" width=\"" << _media_info.video_width
			<< "\" height=\"" << _media_info.video_height
			<< "\" par=\"" << _mpd_pixel_aspect_ratio << "\" frameRate=\"" << _media_info.video_framerate
			<< "\" segmentAlignment=\"true\" startWithSAP=\"1\" subsegmentAlignment=\"true\" subsegmentStartsWithSAP=\"1\">"
			<< "\n"
			<< "\t\t<SegmentTemplate timescale=\"" << _media_info.video_timescale
			<< "\" initialization=\"" << DASH_MPD_VIDEO_INIT_FILE_NAME << "\" media=\"" << _segment_prefix.c_str() << "_$Time$" << DASH_MPD_VIDEO_SUFFIX << "\">\n"
			<< "\t\t\t<SegmentTimeline>\n"
			<< video_urls.c_str()
			<< "\t\t\t</SegmentTimeline>\n"
			<< "\t\t</SegmentTemplate>\n"
			<< "\t\t<Representation codecs=\"avc1.42401f\" sar=\"1:1\" bandwidth=\"" << _media_info.video_bitrate
			<< "\" />\n"
			<< "\t</AdaptationSet>\n";
	}

	// audio listing
	if (!audio_urls.empty())
	{
		play_list_stream << "\t<AdaptationSet id=\"1\" group=\"2\" mimeType=\"audio/mp4\" lang=\"und\" segmentAlignment=\"true\" startWithSAP=\"1\" subsegmentAlignment=\"true\" subsegmentStartsWithSAP=\"1\">\n"
			<< "\t\t<AudioChannelConfiguration schemeIdUri=\"urn:mpeg:dash:23003:3:audio_channel_configuration:2011\" value=\""
			<< _media_info.audio_channels << "\"/>\n"
			<< "\t\t<SegmentTemplate timescale=\"" << _media_info.audio_timescale
			<< "\" initialization=\"" << DASH_MPD_AUDIO_INIT_FILE_NAME << "\" media=\"" << _segment_prefix.c_str() << "_$Time$" << DASH_MPD_AUDIO_SUFFIX << "\">\n"
			<< "\t\t\t<SegmentTimeline>\n"
			<< audio_urls.c_str()
			<< "\t\t\t</SegmentTimeline>\n"
			<< "\t\t</SegmentTemplate>\n"
			<< "\t\t<Representation codecs=\"mp4a.40.2\" audioSamplingRate=\"" << _media_info.audio_samplerate
			<< "\" bandwidth=\"" << _media_info.audio_bitrate << "\" />\n"
			<< "\t</AdaptationSet>\n";
	}

	play_list_stream << "</Period>\n" << "</MPD>\n";

	std::string playlist = play_list_stream.str().c_str();
	SetPlayList(playlist);

	if (_streaming_type == PacketizerStreamingType::Both && _streaming_start)
	{
		if (video_urls.empty())
			LOG_WARNING_WRITE(("Dash video segment urls empty - stream(%s/%s)", _app_name.c_str(), _stream_name.c_str()));

		if (audio_urls.empty())
			LOG_WARNING_WRITE(("Dash audio segment urls empty - stream(%s/%s)", _app_name.c_str(), _stream_name.c_str()));
	}

	return true;
}

//====================================================================================================
// Get Segment
//====================================================================================================
const std::shared_ptr<SegmentInfo> DashPacketizer::GetSegmentData(const std::string& file_name)
{
	if (!_streaming_start)
		return nullptr;

	const auto& file_type = GetFileType(file_name);

	if (file_type == DashFileType::VideoSegment)
	{
		// video segment mutex
		std::unique_lock<std::mutex> lock(_video_segment_guard);

		auto item = std::find_if(_video_segment_datas.begin(), _video_segment_datas.end(),
			[&](std::shared_ptr<SegmentInfo> const& value) -> bool
			{
				return value != nullptr ? value->file_name == file_name : false;
			});

		return (item != _video_segment_datas.end()) ? (*item) : nullptr;
	}
	else if (file_type == DashFileType::AudioSegment)
	{
		// audio segment mutex
		std::unique_lock<std::mutex> lock(_audio_segment_guard);

		auto item = std::find_if(_audio_segment_datas.begin(), _audio_segment_datas.end(),
			[&](std::shared_ptr<SegmentInfo> const& value) -> bool
			{
				return value != nullptr ? value->file_name == file_name : false;
			});

		return (item != _audio_segment_datas.end()) ? (*item) : nullptr;
	}
	else if (file_type == DashFileType::VideoInit)
	{
		return _video_init_file;
	}
	else if (file_type == DashFileType::AudioInit)
	{
		return _audio_init_file;
	}

	return nullptr;
}

//====================================================================================================
// Set Segment
//====================================================================================================
bool DashPacketizer::SetSegmentData(std::string file_name,
									uint64_t duration,
									uint64_t timestamp,
									std::shared_ptr<std::vector<uint8_t>>& data)
{
	const auto& file_type = GetFileType(file_name);

	if (file_type == DashFileType::VideoSegment)
	{
		// video segment mutex
		std::unique_lock<std::mutex> lock(_video_segment_guard);

		_video_segment_datas[_current_video_index++] = std::make_shared<SegmentInfo>(_sequence_number++,
																					file_name,
																					duration,
																					timestamp,
																					data);

		if (_segment_save_count <= _current_video_index)
			_current_video_index = 0;

		_video_sequence_number++;

		LOG_INFO_WRITE(("Dash video segment add - stream(%s/%s) file(%s) duration(%lld/%0.3f)",
			_app_name.c_str(), _stream_name.c_str(), file_name.c_str(), duration, (double)duration / _media_info.video_timescale));

	}
	else if (file_type == DashFileType::AudioSegment)
	{
		// audio segment mutex
		std::unique_lock<std::mutex> lock(_audio_segment_guard);

		_audio_segment_datas[_current_audio_index++] = std::make_shared<SegmentInfo>(_sequence_number++,
																					file_name,
																					duration,
																					timestamp,
																					data);

		if (_segment_save_count <= _current_audio_index)
			_current_audio_index = 0;

		_audio_sequence_number++;

		LOG_DEBUG_WRITE(("Dash audio segment add - stream(%s/%s) file(%s) duration(%lld/%0.3f)",
			_app_name.c_str(), _stream_name.c_str(), file_name.c_str(), duration, (double)duration / _media_info.audio_timescale));
	}

	if (!_streaming_start && (_video_sequence_number > _segment_count || _audio_sequence_number > _segment_count))
	{
		_streaming_start = true;
		LOG_INFO_WRITE(("Dash segment ready completed - stream(%s/%s) segment(%ds/%d)",
			_app_name.c_str(), _stream_name.c_str(), _segment_duration, _segment_count));
	}

	return true;
}

