//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "../packetizer/ts_writer.h"

//====================================================================================================
// HlsPacketizer
//====================================================================================================
class HlsPacketizer : public Packetizer
{
public:
	HlsPacketizer(const std::string& app_name,
				const std::string& stream_name,
				PacketizerStreamingType streaming_type,
				const std::string& segment_prefix,
				uint32_t segment_duration,
				uint32_t segment_count,
				MediaInfo& media_info);

	~HlsPacketizer() = default;

public:
	std::shared_ptr<std::vector<uint8_t>> HlsPacketizer::MakeAdtsHeader(int sample_index, int channel, int frame_size);

	virtual bool AppendVideoFrame(std::shared_ptr<FrameInfo>& frame) override;

	virtual bool AppendAudioFrame(std::shared_ptr<FrameInfo>& frame) override;

	virtual const std::shared_ptr<SegmentInfo> GetSegmentData(const std::string& file_name) override;

	virtual bool SetSegmentData(std::string file_name,
								uint64_t duration,
								uint64_t timestamp,
								std::shared_ptr<std::vector<uint8_t>>& data) override;

	bool SegmentWrite(uint64_t start_timestamp, uint64_t duration);

protected:
	bool UpdatePlaylist();

protected:
	std::vector<std::shared_ptr<FrameInfo>> _frame_list;
	double _duration_margen;

	time_t _last_video_append_time;
	time_t _last_audio_append_time;
	bool _audio_enable;
	bool _video_enable;

	// 0001 + sps + 0001 + pps + 0001
	std::unique_ptr<std::vector<uint8_t>> _key_frame_header = nullptr;
};

