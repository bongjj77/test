//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "../media_define.h"

//====================================================================================================
// TsWriter
//====================================================================================================

class TsWriter
{
public:
	TsWriter(bool video_enable, bool audio_enable);
	virtual ~TsWriter() = default;

public:
	bool WriteSample(bool is_video,
					bool is_keyframe,
					bool aud_insert,
					uint32_t timescale,
					uint64_t timestamp,
					uint64_t cts,
					std::shared_ptr<std::vector<uint8_t>>& data,
					std::shared_ptr<std::vector<uint8_t>> user_buffer = nullptr);

	std::shared_ptr<std::vector<uint8_t>> GetDataStream()
	{
		return _data_stream;
	}
	
	void Clear();

	const std::shared_ptr<std::vector<uint8_t>> & GetAud() { return _access_unit_delimiter; }


	// user buffer support
	std::shared_ptr < std::vector<uint8_t>> GetInitBuffer();


protected:
	static uint32_t	MakeCrc(const uint8_t* data, uint32_t data_size);
	std::unique_ptr<std::vector<uint8_t>> MakePat();
	std::unique_ptr<std::vector<uint8_t>> MakePmt();
	std::shared_ptr<std::vector<uint8_t>> MakePesHeader(int data_size, bool is_video, uint64_t timestamp, uint64_t cts);

	std::unique_ptr<std::vector<uint8_t>> MakeTsHeader(int pid,
														uint32_t continuity_count,
														bool payload_start,
														uint32_t& payload_size,
														bool use_pcr,
														uint64_t pcr,
														bool is_keyframe);


	bool WriteDataStream(int data_size, const uint8_t* data);

	bool WriteDataStream(const std::shared_ptr<std::vector<uint8_t>> & data);
	bool WriteDataStream(const std::unique_ptr<std::vector<uint8_t>> & data);

protected:
	bool _video_enable;
	bool _audio_enable;

	std::unique_ptr<std::vector<uint8_t>> _pat_data;
	std::unique_ptr<std::vector<uint8_t>> _pmt_data;

	std::shared_ptr<std::vector<uint8_t>> _data_stream;
	uint32_t _audio_continuity_count;
	uint32_t _video_continuity_count;

	std::shared_ptr<std::vector<uint8_t>> _access_unit_delimiter;


};