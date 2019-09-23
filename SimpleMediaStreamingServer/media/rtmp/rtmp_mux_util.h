//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once

#include "rtmp_define.h"

#pragma pack(1)
//====================================================================================================
// RtmpMuxMessageHeader
//====================================================================================================
struct RtmpMuxMessageHeader
{
public :
	RtmpMuxMessageHeader() : 	chunk_stream_id(0),
								timestamp(0),
								type_id(0),
								stream_id(0),
								body_size(0)
	{

	}

	RtmpMuxMessageHeader(	uint32_t	chunk_stream_id_,
							uint32_t	timestamp_,
							uint8_t		type_id_,
							uint32_t	stream_id_,
							uint32_t	body_size_)
	{
		this->chunk_stream_id	= chunk_stream_id_;
		this->timestamp			= timestamp_;
		this->type_id			= type_id_;
		this->stream_id			= stream_id_;
		this->body_size			= body_size_;
	}

public :
	uint32_t	chunk_stream_id;
	uint32_t	timestamp;
	uint8_t		type_id;
	uint32_t	stream_id;
	uint32_t	body_size;
};
#pragma pack()
//====================================================================================================
// RtmpMuxUtil
//====================================================================================================
class RtmpMuxUtil
{
public:
						RtmpMuxUtil() = default;
	virtual 			~RtmpMuxUtil() = default;

public:
	static uint8_t	ReadInt8(void * data);
	static uint16_t	ReadInt16(void * data);
	static uint32_t	ReadInt24(void * data);
	static uint32_t	ReadInt32(void * data);
	static int		ReadInt32LE(void * data);

	static int WriteInt8(void * output, uint8_t value);
	static int WriteInt16(void * output, int16_t value);
	static int WriteInt24(void * output, int value);
	static int WriteInt32(void * output, int value);
	static int WriteInt32LE(void * output, int value);

	static bool WriteInt8(std::shared_ptr<std::vector<uint8_t>> &output, uint8_t value);
	static bool WriteInt16(std::shared_ptr<std::vector<uint8_t>> &output, int16_t value);
	static bool WriteInt24(std::shared_ptr<std::vector<uint8_t>> &output, int value);
	static bool WriteInt32(std::shared_ptr<std::vector<uint8_t>> &output, int value);
	static bool WriteInt32LE(std::shared_ptr<std::vector<uint8_t>> &output, int value);

	static int GetBasicHeaderSizeByRawData(uint8_t nData);
	static int GetBasicHeaderSizeByChunkStreamID(uint32_t chunk_stream_id);

	static int GetChunkHeaderSize(void * raw_data, int raw_data_size); // ret:길이, ret<=0:실패
	static std::shared_ptr<RtmpChunkHeader> GetChunkHeader(void * raw_data, int raw_data_size,  int &chunk_header_size, bool &is_extend_type); // ret<=0:실패, ret>0:처리길이

	static std::shared_ptr<std::vector<uint8_t>> MakeChunkBasicHeader(RtmpChunkFormat chunk_format, uint32_t chunk_stream_id);
	static std::shared_ptr<std::vector<uint8_t>> MakeChunkRawHeader(std::shared_ptr<RtmpChunkHeader> &chunk_header, bool is_extend_type);
	static int GetChunkDataRawSize(int chunk_size, uint32_t chunk_stream_id, int chunk_data_size, bool is_extend_type);
	static std::shared_ptr<std::vector<uint8_t>> MakeChunkRawData(int chunk_size, 
																uint32_t chunk_stream_id, 
																std::shared_ptr<std::vector<uint8_t>> &chunk_data, 
																bool is_extend_type, 
																uint32_t time);
};

