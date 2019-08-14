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
// Frame Info
//===============================================================================================
struct FrameInfo
{
public:
	FrameInfo(uint32_t timestamp_, int cts_, FrameType frame_type_, int frame_size_, uint8_t* frame_)
	{
		timestamp	= timestamp_;
		cts			= cts_;
		frame_type	= frame_type_;
		frame_data	= std::make_shared<std::vector<uint8_t>>(frame_, frame_ + frame_size_);
	}

	FrameInfo(uint32_t timestamp_, int cts_, FrameType frame_type_, std::shared_ptr<std::vector<uint8_t>> frame_data_)
	{
		timestamp	= timestamp_;
		cts			= cts_;
		frame_type	= frame_type_;
		frame_data	= frame_data_;
	}

public:
	uint64_t	timestamp; // dts or pts 
	uint64_t	cts; // cts;
	FrameType	frame_type;

	std::shared_ptr<std::vector<uint8_t>> frame_data;
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

		timestamp_scale = RTMP_TIMESCALE;
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

	uint32_t	timestamp_scale;
	EncoderType encoder_type;

	//h.264 AVC Info
	std::shared_ptr<std::vector<uint8_t>> avc_sps;
	std::shared_ptr<std::vector<uint8_t>> avc_pps;
};

#pragma pack()
