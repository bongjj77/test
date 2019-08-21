//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once

#include "../packetyzer/packetyzer.h"
#include "../packetyzer/m4s_init_writer.h"
#include "../packetyzer/m4s_segment_writer.h"

enum class DashFileType : int32_t
{
	Unknown,
	PlayList,
	VideoInit,
	AudioInit,
	VideoSegment,
	AudioSegment,
};

//====================================================================================================
// DashPacketyzer
//====================================================================================================
class DashPacketyzer : public Packetyzer
{
public:
	DashPacketyzer(const std::string& app_name,
				const std::string& stream_name,
				PacketyzerStreamingType stream_type,
				const std::string& segment_prefix,
				uint32_t segment_duration,
				uint32_t segment_count,
				MediaInfo& media_info);

	~DashPacketyzer() final;

public:

	static DashFileType GetFileType(const std::string& file_name);

	bool VideoInit(const std::shared_ptr<std::vector<uint8_t>>& frame_info);

	bool AudioInit();

	virtual bool AppendVideoFrame(std::shared_ptr<FrameInfo>& frame_info) override;

	virtual bool AppendAudioFrame(std::shared_ptr<FrameInfo>& frame_info) override;

	virtual const std::shared_ptr<SegmentInfo> GetSegmentData(const std::string& file_name) override;

	virtual bool SetSegmentData(std::string file_name,
								uint64_t duration,
								uint64_t timestamp,
								std::shared_ptr<std::vector<uint8_t>>& data) override;

	bool VideoSegmentWrite(uint64_t max_timestamp);

	bool AudioSegmentWrite(uint64_t max_timestamp);

protected:

	bool GetSegmentPlayInfos(std::string& video_urls,
		std::string& audio_urls,
		double& time_shift_buffer_depth,
		double& minimumUpdatePeriod);

	bool UpdatePlayList();

private:
	
	std::string _start_time;
	std::string _mpd_pixel_aspect_ratio;
	double _mpd_suggested_presentation_delay;
	double _mpd_min_buffer_time;

	std::shared_ptr<SegmentInfo> _video_init_file = nullptr;
	std::shared_ptr<SegmentInfo> _audio_init_file = nullptr;

	uint32_t _video_sequence_number = 1;
	uint32_t _audio_sequence_number = 1;

	std::deque<std::shared_ptr<FrameInfo>> _video_frrame_list;
	std::deque<std::shared_ptr<FrameInfo>> _audio_frame_list;

	time_t _last_video_append_time;
	time_t _last_audio_append_time;

	double _duration_margen;
};