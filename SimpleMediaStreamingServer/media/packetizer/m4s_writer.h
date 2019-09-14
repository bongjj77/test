//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once

#include <vector>
#include <deque>
#include <string>
#include <memory>
#include "..//media_define.h"
#include "common/common_header.h"

#define M4S_BOX_HEADER_SIZE  (8)        // size(4) + type(4)
#define M4S_BOX_EXT_HEADER_SIZE (12)    // size(4) + type(4) + version(1) + flag(3)

enum class M4sMediaType
{
	Video,
	Audio,
	Data,
};

//====================================================================================================
// Fragment Sample Data
//====================================================================================================
struct SampleData
{
public:
	SampleData(uint64_t duration_,
                       uint32_t flag_,
                       uint64_t timestamp_,
                       uint32_t composition_time_offset_,
                       std::shared_ptr<std::vector<uint8_t>> &data_)
    {
        duration                =  duration_;
        flag                    = flag_;
        timestamp               = timestamp_;
        composition_time_offset = composition_time_offset_;
        data		            = data_;
    }

	SampleData(uint64_t duration_,
			   uint64_t timestamp_,
			   std::shared_ptr<std::vector<uint8_t>> &data_)
	{
		duration                =  duration_;
		flag                    = 0;
		timestamp               = timestamp_;
		composition_time_offset = 0;
		data		            = data_;
	}


public:
    uint64_t duration;
    uint32_t flag;
    uint64_t timestamp;
    uint32_t composition_time_offset;
    std::shared_ptr<std::vector<uint8_t>> data;
};

//====================================================================================================
// M4sWriter
//====================================================================================================
class M4sWriter
{
public:
	M4sWriter(M4sMediaType media_type);
	virtual ~M4sWriter() = default;

protected :

	bool WriteData(const std::shared_ptr<std::vector<uint8_t>> &data, std::shared_ptr<std::vector<uint8_t>> &data_stream);
	bool WriteData(const std::vector<uint8_t>& data, std::shared_ptr<std::vector<uint8_t>>& data_stream);
  	bool WriteText(std::string value, std::shared_ptr<std::vector<uint8_t>> &data_stream);
	bool WriteInit(uint8_t value, int init_size, std::shared_ptr<std::vector<uint8_t>> &data_stream);
	bool WriteUint64(uint64_t value, std::shared_ptr<std::vector<uint8_t>> &data_stream);
	bool WriteUint32(uint32_t value, std::shared_ptr<std::vector<uint8_t>> &data_stream);
	bool WriteUint24(uint32_t value, std::shared_ptr<std::vector<uint8_t>> &data_stream);
	bool WriteUint16(uint16_t value, std::shared_ptr<std::vector<uint8_t>> &data_stream);
	bool WriteUint8(uint8_t value, std::shared_ptr<std::vector<uint8_t>> &data_stream);

	int BoxDataWrite(std::string type,
					const std::shared_ptr<std::vector<uint8_t>> &data,
					std::shared_ptr<std::vector<uint8_t>> &data_stream);

	int BoxDataWrite(std::string type,
					uint8_t version,
					uint32_t flags,
					const std::shared_ptr<std::vector<uint8_t>> &data,
					std::shared_ptr<std::vector<uint8_t>> &data_stream);

	int BoxDataWrite(const std::string type,
					const std::shared_ptr<std::vector<uint8_t>>& data,
					std::shared_ptr<std::vector<uint8_t>>& data_stream,
					bool data_size_write);


protected :
	M4sMediaType _media_type;
};