﻿//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once

#include "m4s_writer.h"

//====================================================================================================
// CmafChunkWriter
//====================================================================================================
class CmafChunkWriter : public M4sWriter
{
public:
	CmafChunkWriter(M4sMediaType media_type,
					uint32_t sequence_number,
					uint32_t track_id,
					bool http_chunked_transfer_support);

	~CmafChunkWriter() final;

public :
    const std::shared_ptr<std::vector<uint8_t>> AppendSample(const std::shared_ptr<SampleData> &sample_data);

    uint32_t GetSequenceNumber(){ return _sequence_number; }
    uint64_t GetStartTimestamp(){ return _start_timestamp; }

    bool IsWriteStarted(){ return _write_started; }
    void Clear()
    {
        _write_started = false;
        _start_timestamp = 0;
		_sample_count = 0;
		_chunked_data = nullptr;
    }

	 std::shared_ptr<std::vector<uint8_t>> GetChunkedSegment();
	uint32_t GetSampleCount(){ return _sample_count; }
protected :

	int MoofBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream, const std::shared_ptr<SampleData> &sample_data);
	int MfhdBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream);
	int TrafBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream, const std::shared_ptr<SampleData> &sample_data);

	int TfhdBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream);
	int TfdtBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream, uint64_t timestamp);
	int TrunBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream, const std::shared_ptr<SampleData> &sample_data);

	int MdatBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream, const std::shared_ptr<std::vector<uint8_t>> &frame_info);

private :

	 std::shared_ptr<std::vector<uint8_t>> _chunked_data = nullptr;
	 uint32_t _max_chunked_data_size = 100*1024;

	bool _http_chunked_transfer_support = true;

	uint32_t _sequence_number;
	uint32_t _track_id;

	bool _write_started = false;
	uint64_t _start_timestamp = 0;
	uint32_t _sample_count = 0;

};