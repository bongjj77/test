//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "rtmp_mux_util.h" 

uint8_t RtmpMuxUtil::ReadInt8(void * data)
{
	uint8_t return_value;

	return_value = ((uint8_t*)data)[0];

	return return_value;
}

uint16_t RtmpMuxUtil::ReadInt16(void * data)
{
	uint16_t return_value;

	return_value = (((uint8_t*)data)[0] << 8) | ((uint8_t*)data)[1];

	return return_value;
}

uint32_t RtmpMuxUtil::ReadInt24(void * data)
{
	uint32_t return_value;

	return_value = (((uint8_t*)data)[0] << 16) | (((uint8_t*)data)[1] << 8) | ((uint8_t*)data)[2];

	return return_value;
}

uint32_t RtmpMuxUtil::ReadInt32(void * data)
{
	uint32_t return_value;

	return_value = (((uint8_t*)data)[0] << 24) | (((uint8_t*)data)[1] << 16) | (((uint8_t*)data)[2] << 8) | ((uint8_t*)data)[3];

	return return_value;
}

int RtmpMuxUtil::ReadInt32LE(void * data)
{
	int return_value;

	memcpy(&return_value, data, sizeof(int));

	return return_value;
}

int RtmpMuxUtil::WriteInt8(void * output, uint8_t value)
{
	((uint8_t*)output)[0] = value;

	return sizeof(uint8_t);
}

int RtmpMuxUtil::WriteInt16(void * output, int16_t value)
{
	((uint8_t*)output)[1] = (uint8_t)(value & 0xff);
	((uint8_t*)output)[0] = (uint8_t)(value >> 8);
	return sizeof(int16_t);
}

int RtmpMuxUtil::WriteInt24(void * output, int value)
{
	((uint8_t*)output)[2] = (uint8_t)(value & 0xff);
	((uint8_t*)output)[1] = (uint8_t)(value >> 8);
	((uint8_t*)output)[0] = (uint8_t)(value >> 16);
	return 3;
}

int RtmpMuxUtil::WriteInt32(void * output, int value)
{
	((uint8_t*)output)[3] = (uint8_t)(value & 0xff);
	((uint8_t*)output)[2] = (uint8_t)(value >> 8);
	((uint8_t*)output)[1] = (uint8_t)(value >> 16);
	((uint8_t*)output)[0] = (uint8_t)(value >> 24);
	return sizeof(int);
}

int RtmpMuxUtil::WriteInt32LE(void * output, int value)
{
	memcpy(output, &value, sizeof(int));
	return sizeof(int);
}

bool RtmpMuxUtil::WriteInt8(std::shared_ptr<std::vector<uint8_t>> &output, uint8_t value)
{
	output->push_back(value);
	return true;
}

bool RtmpMuxUtil::WriteInt16(std::shared_ptr<std::vector<uint8_t>> &output, int16_t value)
{
	
	output->push_back((uint8_t)(value >> 8));
	output->push_back((uint8_t)(value & 0xff));
	return true;
}

bool RtmpMuxUtil::WriteInt24(std::shared_ptr<std::vector<uint8_t>> &output, int value)
{
	output->push_back((uint8_t)(value >> 16));
	output->push_back((uint8_t)(value >> 8));
	output->push_back((uint8_t)(value & 0xff));
	
	return true;
}

bool RtmpMuxUtil::WriteInt32(std::shared_ptr<std::vector<uint8_t>> &output, int value)
{

	output->push_back((uint8_t)(value >> 24));
	output->push_back((uint8_t)(value >> 16));
	output->push_back((uint8_t)(value >> 8));
	output->push_back((uint8_t)(value & 0xff));

	return true;
}

bool RtmpMuxUtil::WriteInt32LE(std::shared_ptr<std::vector<uint8_t>> &output, int value)
{
	output->insert(output->end(), &value, &value + sizeof(int));
	return true;
}


//====================================================================================================
// GetBasicHeaderSizeByRawData
// basic_header 크기 획득(RawData) 
// FormatType + CSID 
//====================================================================================================
int RtmpMuxUtil::GetBasicHeaderSizeByRawData(uint8_t data)
{
	int header_size = 1; 
	
	switch(data & RTMP_CHUNK_BASIC_CHUNK_STREAM_ID_MASK)
	{
		case 0:		header_size = 2; 	break;
		case 1:		header_size = 3; 	break;
		default : 	header_size = 1;	break;
	}
	return header_size;
}

//====================================================================================================
// GetBasicHeaderSizeByChunkStreamID
// basic_header 크기 획득 (CSID)
// FormatType + CSID 
//====================================================================================================
int RtmpMuxUtil::GetBasicHeaderSizeByChunkStreamID(uint32_t chunk_stream_id)
{
	int	header_size = 1;
 
	if		( chunk_stream_id >= (64+256) )	header_size = 3;
	else if( chunk_stream_id >= 64 )		header_size = 2;
	else									header_size = 1;

	return header_size;
}

//====================================================================================================
// GetChunkHeaderSize
// Header size ( basic_header + message_header)
// - Type3는 확장 헤더 확인 못함 - 이전 정보를 기반으로 이후에 확인
//====================================================================================================
int RtmpMuxUtil::GetChunkHeaderSize(void * raw_data, int raw_data_size)
{
	int		    basic_header_size	= 0;
	int		    message_header_size	= 0;
	int		    header_size			= 0;
	auto *	    data_pos 		= (uint8_t*)raw_data;
	
	if(raw_data == nullptr || raw_data_size == 0) 
	{ 
		return 0; 
	}
 
	auto format = (RtmpChunkFormat)(data_pos[0] & RTMP_CHUNK_BASIC_FORMAT_TYPE_MASK);
	auto chunk_stream_id	= (uint32_t)(data_pos[0] & RTMP_CHUNK_BASIC_CHUNK_STREAM_ID_MASK);

	// basic_header length 
	if ( chunk_stream_id == 0 )			basic_header_size = 2;
	else if	( chunk_stream_id == 1 )	basic_header_size = 3;
	else 								basic_header_size = 1;
		
	// msg header length
	switch(format)
	{
		case RtmpChunkFormat::Type_0:	message_header_size = RTMP_CHUNK_TYPE_0_SIZE;	break;
		case RtmpChunkFormat::Type_1:	message_header_size = RTMP_CHUNK_TYPE_1_SIZE;	break;
		case RtmpChunkFormat::Type_2:	message_header_size = RTMP_CHUNK_TYPE_2_SIZE;	break;
		case RtmpChunkFormat::Type_3:	message_header_size = RTMP_CHUNK_TYPE_3_SIZE;	break;
		default : 							return -1;
	}

	header_size = basic_header_size + message_header_size; 
 
	if(raw_data_size < header_size)
	{
		return 0; 
	}	

	//Extended Timestamp 확인(Type3는 확인 못함 - 이전 정보를 기반으로 이후에 확인) 
	if(format == RtmpChunkFormat::Type_0 || format == RtmpChunkFormat::Type_1 || format == RtmpChunkFormat::Type_2)
	{
		if( ReadInt24(data_pos + basic_header_size) == RTMP_EXTENDED_FORMAT )
		{ 
			header_size += RTMP_EXTEND_TIMESTAMP_SIZE; 
 
			if(raw_data_size < header_size)
			{
				return 0; 
			}
			
		}
	}

	return  header_size;
}


//====================================================================================================
// GetChunkHeader
// Chunk 헤더 획득 
// - Type3는 확장 헤더 확인 못함 - 이전 정보를 기반으로 이후에 확인
//====================================================================================================
std::shared_ptr<RtmpChunkHeader> RtmpMuxUtil::GetChunkHeader(void * raw_data, 
															int raw_data_size,  
															int &chunk_header_size, 
															bool &is_extend_type)
{
	auto * data_pos = (uint8_t*)raw_data;
	
	chunk_header_size = 0;
	is_extend_type = false;

	if (raw_data == nullptr || raw_data_size == 0)
	{
		return nullptr;
	}

	auto basic_header_size = GetBasicHeaderSizeByRawData(data_pos[0]);
	chunk_header_size = GetChunkHeaderSize(raw_data, raw_data_size);

	if (chunk_header_size <= 0)
	{
		return nullptr;
	}

	auto chunk_header = std::make_shared<RtmpChunkHeader>();

	RtmpChunkFormat format = (RtmpChunkFormat)(data_pos[0] & RTMP_CHUNK_BASIC_FORMAT_TYPE_MASK);

	auto chunk_stream_id = (uint32_t)(data_pos[0] & RTMP_CHUNK_BASIC_CHUNK_STREAM_ID_MASK);

	if (chunk_stream_id == 0) 	chunk_stream_id = (uint32_t)(64 + data_pos[1]);
	else if (chunk_stream_id == 1)	chunk_stream_id = (uint32_t)(64 + data_pos[1] + data_pos[2] * 256);

	chunk_header->basic_header.format_type = format;
	chunk_header->basic_header.chunk_stream_id = chunk_stream_id;

	data_pos += basic_header_size;

	switch (format)
	{
	case RtmpChunkFormat::Type_0:
	{
		chunk_header->type_0.timestamp = ReadInt24(data_pos);
		data_pos += 3;

		chunk_header->type_0.body_size = ReadInt24(data_pos);
		data_pos += 3;

		chunk_header->type_0.type_id = data_pos[0];
		data_pos += 1;

		chunk_header->type_0.stream_id = (uint32_t)ReadInt32LE(data_pos);
		data_pos += 4;

		//Externed Timestamp check 
		if (chunk_header->type_0.timestamp == RTMP_EXTENDED_FORMAT)
		{
			chunk_header->type_0.timestamp = ReadInt32(data_pos);
			is_extend_type = true;
		}
		break;
	}
	case RtmpChunkFormat::Type_1:
	{
		chunk_header->type_1.timestamp_delta = ReadInt24(data_pos);
		data_pos += 3;

		chunk_header->type_1.body_size = ReadInt24(data_pos);
		data_pos += 3;

		chunk_header->type_1.type_id = data_pos[0];
		data_pos += 1;

		// Externed Timestamp ceck
		if (chunk_header->type_1.timestamp_delta == RTMP_EXTENDED_FORMAT)
		{
			chunk_header->type_1.timestamp_delta = ReadInt32(data_pos);
			is_extend_type = true;
		}

		break;
	}
	case RtmpChunkFormat::Type_2:
	{
		chunk_header->type_2.timestamp_delta = ReadInt24(data_pos);
		data_pos += 3;

		// Externed Timestamp check
		if (chunk_header->type_2.timestamp_delta == RTMP_EXTENDED_FORMAT)
		{
			chunk_header->type_2.timestamp_delta = ReadInt32(data_pos);
			is_extend_type = true;
		}

		break;
	}
	case RtmpChunkFormat::Type_3:
	{
		break;
	}
	default:
	{
		break;
	}
	}

	return chunk_header;
}

//====================================================================================================
// MakeChunkBasicHeader
//====================================================================================================
std::shared_ptr<std::vector<uint8_t>> RtmpMuxUtil::MakeChunkBasicHeader(RtmpChunkFormat chunk_format, uint32_t chunk_stream_id)
{
	auto basic_header = std::make_shared<std::vector<uint8_t>>(); 
 
	if(basic_header == nullptr)
	{ 
		return 0; 
	}
 
	if(chunk_stream_id >= (64+256))
	{
		basic_header->push_back(1);
		chunk_stream_id  	-= 64;
		basic_header->push_back((uint8_t)(chunk_stream_id&0xff) );
		chunk_stream_id 	>>= 8;
		basic_header->push_back((uint8_t)(chunk_stream_id&0xff));
	}
	else if(chunk_stream_id >= 64)
	{
		basic_header->push_back(0);
		chunk_stream_id  	-= 64;
		basic_header->push_back((uint8_t)(chunk_stream_id & 0xff));
	}
	else
	{
		basic_header->push_back((uint8_t)(chunk_stream_id & 0x3f));
	}
 
	(*basic_header)[0] |= (uint8_t)chunk_format & 0xc0;

	return basic_header;
}

//====================================================================================================
// MakeChunkRawHeader
// - Chunk header create
// - basic_header + message_header
//====================================================================================================
std::shared_ptr<std::vector<uint8_t>> RtmpMuxUtil::MakeChunkRawHeader(std::shared_ptr<RtmpChunkHeader> &chunk_header, bool is_extend_type)
{
	if (chunk_header == nullptr)
	{
		return nullptr;
	}

	auto basic_header = MakeChunkBasicHeader(chunk_header->basic_header.format_type, chunk_header->basic_header.chunk_stream_id);
	auto raw_header = std::make_shared<std::vector<uint8_t>>(basic_header->begin(), basic_header->end());
 	 
	switch(chunk_header->basic_header.format_type)
	{
		case RtmpChunkFormat::Type_0:
		{
			if(!is_extend_type)	WriteInt24(raw_header, chunk_header->type_0.timestamp);
			else				WriteInt24(raw_header, RTMP_EXTENDED_FORMAT);
		
			WriteInt24(raw_header, chunk_header->type_0.body_size);
			raw_header->push_back(chunk_header->type_0.type_id);
			WriteInt32LE(raw_header, chunk_header->type_0.stream_id);
			
			//extended timestamp
			if(is_extend_type)
				WriteInt32(raw_header, chunk_header->type_0.timestamp);
			
			break;
		}
		case RtmpChunkFormat::Type_1:
		{
			if(!is_extend_type)	WriteInt24(raw_header, chunk_header->type_1.timestamp_delta);
			else				WriteInt24(raw_header, RTMP_EXTENDED_FORMAT);
	
			WriteInt24(raw_header, chunk_header->type_1.body_size);
			raw_header->push_back(chunk_header->type_1.type_id);
			
			// extended timestamp
			if(is_extend_type)
				WriteInt32(raw_header, chunk_header->type_1.timestamp_delta);
			
			break;
		}
		case RtmpChunkFormat::Type_2:
		{
			if(!is_extend_type)
			{
				WriteInt24(raw_header, chunk_header->type_2.timestamp_delta);
			}
			else
			{
				WriteInt24(raw_header, RTMP_EXTENDED_FORMAT);
				WriteInt32(raw_header, chunk_header->type_2.timestamp_delta);
			}
			
			break;
		}
		default : break;
	}

	return raw_header;
}

//====================================================================================================
// GetChunkDataRawSize
// - min chunk size calculation
// - chunk header + block + chunk header(type3) + block + chunk header(type3) + block....
//====================================================================================================
int RtmpMuxUtil::GetChunkDataRawSize(int chunk_size, uint32_t chunk_stream_id, int chunk_data_size, bool is_extend_type)
{
	int type3_header_size 	= 0;
	int block_count			= 0; 
	 
	if(chunk_size == 0 || chunk_data_size == 0 )
	{ 
		return 0; 
	}
 
	type3_header_size = GetBasicHeaderSizeByChunkStreamID(chunk_stream_id);

	if(is_extend_type)
	{
		type3_header_size += RTMP_EXTEND_TIMESTAMP_SIZE; //Extended Timestamp Header 정보가 출력 
	}
	
	block_count = (chunk_data_size-1) / chunk_size; 
	 
	return chunk_data_size + block_count*type3_header_size;
}

//====================================================================================================
// MakeChunkRawData 
// - RawData :     [Block] + [Type3 Header] + [Block] + [Type3 Header] + [Block] ..... 구조 
//====================================================================================================
std::shared_ptr<std::vector<uint8_t>> RtmpMuxUtil::MakeChunkRawData(int chunk_size,
																	uint32_t chunk_stream_id, 
																	std::shared_ptr<std::vector<uint8_t>> &chunk_data, 
																	bool is_extend_type, 
																	uint32_t time)
{
	int read_data_size = 0;
	 
	if(chunk_size == 0 || chunk_data->empty())
	{ 
		return nullptr; 
	}
 
	auto block_count = (chunk_data->size() - 1)/chunk_size;
	block_count++; 
	 
	auto basic_header = MakeChunkBasicHeader(RtmpChunkFormat::Type_3, chunk_stream_id);
	auto chunk_raw_data = std::make_shared<std::vector<uint8_t>>();

	for(int index = 0 ; index < block_count ; index++)
	{
		if(index > 0 )
		{
			chunk_raw_data->insert(chunk_raw_data->end(), basic_header->begin(), basic_header->end());
	
			//Timestamp Extended Header
			if(is_extend_type)
			{
				WriteInt32(chunk_raw_data, time);
			}
		}
 
        auto size = (static_cast<int>(chunk_data->size()) - read_data_size) > chunk_size ?
                chunk_size : (static_cast<int>(chunk_data->size()) - read_data_size);
 
		chunk_raw_data->insert(chunk_raw_data->end(), chunk_data->data() + read_data_size, chunk_data->data() + read_data_size + size);

		read_data_size	+= size;		
	}

	return chunk_raw_data;
}

