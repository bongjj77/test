//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once

#include "common/common_header.h"

#pragma pack(1)

#define RTMP_TIMESCALE			(1000)
#define HLS_TIMESCALE			(90000)
#define DASH_VIDEO_TIMESCALE	(90000)

#define AVC_NAL_HEADER_SIZE      (4) // 00 00 00 01  or 00 00 01
#define ADTS_HEADER_SIZE         (7)
#define MAX_MEDIA_PACKET_SIZE          (20*1024*1024) // 20M

// Avc Nal Header 
const char g_avc_nal_header[AVC_NAL_HEADER_SIZE] = { 0, 0, 0, 1 };

// samplerate index table
const int g_sample_rate_table[] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0 };
#define SAMPLERATE_TABLE_SIZE (16)

// StreamKey app/stream 
#define StreamKey std::pair< std::string, std::string> 


//===============================================================================================
// Playlist Type
//===============================================================================================
enum class PlaylistType : int32_t
{
	M3u8 = 0,	// hls
	Mpd,		// dash
};

//===============================================================================================
// Segment Type
//===============================================================================================
enum class SegmentType : int32_t
{
	Ts,		// hls - mpeg ts
	M4s,	// hls or dash  - fMP4
};

//===============================================================================================
// Rtmp Encoder Type
//===============================================================================================
enum class EncoderType : int32_t
{
	Custom = 0,	// 일반 Rtmp 클라이언트(기타 구분 불가능 Client 포함)
	Xsplit,     // XSplit
	OBS,        // OBS
	Lavf,       // Libavformat (lavf)
};

//===============================================================================================
// Frame Type
//===============================================================================================
enum class FrameType : int32_t
{
	VideoKeyFrame = 'I',// VIDEO Key(I) Frame
	VideoPFrame = 'P',	// VIDEO P Frame
	AudioFrame = 'A',	// AUDIO Frame
};

//===============================================================================================
// Codec Type
//===============================================================================================
enum class CodecType : int32_t
{
	Unknown,
	H264,	//	H264/X264 avc1(7)
	AAC,    //	AAC          mp4a(10)
	MP3,	//	MP3(2)
	SPEEX,	//	SPEEX(11)
};

//===============================================================================================
// Codec Type
//===============================================================================================
enum class PacketyzerStreamingType : int32_t
{
	Both,		// Video + Audio
	VideoOnly,
	AudioOnly,
};

enum class PacketyzerType : int32_t
{
	Dash,
	Hls,
	Cmaf,
};

enum class SegmentDataType : int32_t
{
	Ts,        // Video + Audio
	Mp4Video,
	Mp4Audio,
};

//===============================================================================================
// Frame Info
//===============================================================================================
struct FrameInfo
{
public:
	FrameInfo(uint32_t timestamp_, int cts_, FrameType type_, int size_, uint8_t* data_)
	{
		timestamp = timestamp_;
		cts = cts_;
		type = type_;
		data = std::make_shared<std::vector<uint8_t>>(data_, data_ + size_);
		timescale = 0;
	}

	FrameInfo(uint32_t timestamp_, int cts_, FrameType type_, std::shared_ptr<std::vector<uint8_t>> data_)
	{
		timestamp = timestamp_;
		cts = cts_;
		type = type_;
		data = data_;
		timescale = 0;
	}

public:
	uint64_t	timestamp; // dts or pts 
	uint64_t	cts; // cts;
	FrameType	type;
	uint32_t	timescale; 
	std::shared_ptr<std::vector<uint8_t>> data;
};

//===============================================================================================
// Media Info
//===============================================================================================
struct MediaInfo
{
public:
	MediaInfo()
	{
		video_streaming = false;
		audio_streaming = false;

		// video info
		video_codec_type = CodecType::Unknown;
		video_width = 0;
		video_height = 0;
		video_framerate = 0;
		video_bitrate = 0;

		// audio info
		audio_codec_type = CodecType::Unknown;
		audio_channels = 0;
		audio_bits = 0;
		audio_samplerate = 0;
		audio_sampleindex = 0;
		audio_bitrate = 0;

		video_timescale = RTMP_TIMESCALE;
		audio_timescale = RTMP_TIMESCALE;
		encoder_type = EncoderType::Custom;

		//h.264 AVC Info
		avc_sps = std::make_shared<std::vector<uint8_t>>();
		avc_pps = std::make_shared<std::vector<uint8_t>>();
	}

public:
	bool		video_streaming;
	bool		audio_streaming;

	// Video Info
	CodecType	video_codec_type;
	int			video_width;
	int			video_height;
	float		video_framerate;
	int			video_bitrate;

	// Audio Info
	CodecType	audio_codec_type;
	int			audio_channels;
	int			audio_bits;
	int			audio_samplerate;
	int			audio_sampleindex;
	int			audio_bitrate;

	uint32_t	video_timescale;
	uint32_t	audio_timescale;
	EncoderType encoder_type;

	//h.264 AVC Info
	std::shared_ptr<std::vector<uint8_t>> avc_sps;
	std::shared_ptr<std::vector<uint8_t>> avc_pps;
};


//====================================================================================================
// SegmentInfo
//====================================================================================================
struct SegmentInfo
{
public:
	SegmentInfo(int sequence_number_,
		std::string file_name_,
		uint64_t duration_,
		uint64_t timestamp_,
		std::shared_ptr<std::vector<uint8_t>>& data_)
	{
		sequence_number = sequence_number_;
		file_name = file_name_;
		create_time = time(nullptr);
		duration = duration_;
		timestamp = timestamp_;
		data = data_;
	}

public:
	int			sequence_number;
	std::string file_name;
	time_t		create_time;
	uint64_t	duration;
	uint64_t	timestamp;
	std::shared_ptr<std::vector<uint8_t>> data;
};

#pragma pack()
