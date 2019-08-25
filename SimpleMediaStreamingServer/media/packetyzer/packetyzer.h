//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include <map>
#include "../media_define.h"

//====================================================================================================
// Packetyzer
//====================================================================================================
class Packetyzer
{
public:
    Packetyzer(const std::string &app_name,
			const std::string &stream_name,
			PacketyzerType packetyzer_type,
			PacketyzerStreamingType streaming_type,
			const std::string &segment_prefix,
			uint32_t segment_duration,
			uint32_t segment_count,
			MediaInfo &media_info);

    virtual ~Packetyzer();

public :
    virtual bool AppendVideoFrame(std::shared_ptr<FrameInfo> &frame) = 0;

    virtual bool AppendAudioFrame(std::shared_ptr<FrameInfo> &frame) = 0;

    virtual const std::shared_ptr<SegmentInfo> GetSegmentData(const std::string &file_name) = 0;

    virtual bool SetSegmentData(std::string file_name,
								uint64_t duration,
								uint64_t timestamp,
								std::shared_ptr<std::vector<uint8_t>> &data) = 0;
	
    void SetPlayList(std::string &play_list);

    virtual bool GetPlayList(std::string &play_list);

    bool GetVideoPlaySegments(std::vector<std::shared_ptr<SegmentInfo>> &segment_datas);

    bool GetAudioPlaySegments(std::vector<std::shared_ptr<SegmentInfo>> &segment_datas);

    static std::string MakeUtcSecond(time_t value);
	static std::string MakeUtcMillisecond(double value);
    

protected :
    std::string _app_name;
    std::string _stream_name;
    PacketyzerType _packetyzer_type;
    std::string _segment_prefix;
    PacketyzerStreamingType _streaming_type;

    uint32_t _segment_count;
    uint32_t _segment_save_count;
	uint32_t _segment_duration;  
    MediaInfo _media_info;

    uint32_t _sequence_number;
    bool _streaming_start;
    std::string _play_list;

    bool _video_init;
    bool _audio_init;

    uint32_t _current_video_index = 0;
    uint32_t _current_audio_index = 0;

    std::vector<std::shared_ptr<SegmentInfo>> _video_segment_datas; // m4s : video , ts : video+audio
    std::vector<std::shared_ptr<SegmentInfo>> _audio_segment_datas; // m4s : audio

    std::mutex _video_segment_guard;
    std::mutex _audio_segment_guard;
    std::mutex _play_list_guard;
};
