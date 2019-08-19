﻿//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once

#include "m4s_writer.h"

//====================================================================================================
// M4sSegmentWriter
//====================================================================================================
class M4sSegmentWriter : public M4sWriter
{
public:
	M4sSegmentWriter(M4sMediaType media_type, uint32_t sequence_number, uint32_t track_id, uint64_t start_timestamp);
	~M4sSegmentWriter() final;

public :
	const std::shared_ptr<std::vector<uint8_t>> AppendSamples(const std::vector<std::shared_ptr<SampleData>> &sample_datas);

protected :

	int MoofBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream, const std::vector<std::shared_ptr<SampleData>> &sample_datas);

	int MfhdBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream);

	int TrafBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream, const std::vector<std::shared_ptr<SampleData>> &sample_datas);

	int TfhdBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream);

	int TfdtBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream);

	int TrunBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream, const std::vector<std::shared_ptr<SampleData>> &sample_datas);
		
	int MdatBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream,
					 const std::vector<std::shared_ptr<SampleData>> &sample_datas,
					 uint32_t total_sample_size);
   
private :
    uint32_t _sequence_number;
	uint32_t _track_id;
	uint64_t _start_timestamp;
};