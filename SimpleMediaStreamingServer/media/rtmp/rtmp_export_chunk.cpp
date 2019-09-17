//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "rtmp_export_chunk.h"

//====================================================================================================
// Constructor 
//====================================================================================================
RtmpExportChunk::RtmpExportChunk(bool compress_header, int chunk_size)
{
	_stream_map.clear();
	_compress_header = compress_header;
	_chunk_size = chunk_size;

}

//====================================================================================================
// Destructor 
//====================================================================================================
RtmpExportChunk::~RtmpExportChunk( )
{
	Destroy();
}

//====================================================================================================
// 종료
//====================================================================================================
void RtmpExportChunk::Destroy()
{
	_stream_map.clear();
}

//====================================================================================================
// Export 스트림 획득 
//====================================================================================================
std::shared_ptr<ExportStream> RtmpExportChunk::GetStream(uint32_t chunk_stream_id)
{
	auto item = _stream_map.find(chunk_stream_id);

	if( item != _stream_map.end() )
	{ 
		return item->second; 
	}
 
	auto stream = std::make_shared<ExportStream>();
 
	_stream_map.insert(std::pair<uint32_t, std::shared_ptr<ExportStream>>(chunk_stream_id, stream));

	return stream;
}

//====================================================================================================
// Export 청크 헤더 설정
// - Type3는 확장 헤더 확인 못함 - 이전 정보를 기반으로 이후에 확인
// - 압축 헤더 : 프레임이 정상 적으로 않나오는 상황에서 헤더 압축 사용하면 타임스템프 전달이 정상적으로 않됨 
//====================================================================================================
std::shared_ptr<RtmpChunkHeader> RtmpExportChunk::GetChunkHeader(std::shared_ptr<ExportStream> &stream, 
																std::shared_ptr<RtmpMuxMessageHeader> &message_header, 
																bool &is_extend_type)
{
	auto chunk_header = std::make_shared<RtmpChunkHeader>();

    // type 2
	if(_compress_header &&
	    stream->message_header->chunk_stream_id  == message_header->chunk_stream_id 	&&
		stream->message_header->body_size == message_header->body_size &&
		stream->message_header->stream_id == message_header->stream_id &&
		stream->message_header->type_id == message_header->type_id)
	{
		chunk_header->basic_header.format_type		= RtmpChunkFormat::Type_2;
		chunk_header->basic_header.chunk_stream_id	= message_header->chunk_stream_id;
		chunk_header->type_2.timestamp_delta		= message_header->timestamp - stream->message_header->timestamp;
		
		if(chunk_header->type_2.timestamp_delta >= RTMP_EXTENDED_FORMAT)
		{
			is_extend_type = true; 
		}
	}
	// type 1
	else if(_compress_header &&
	        stream->message_header->chunk_stream_id == message_header->chunk_stream_id &&
	        stream->message_header->stream_id == message_header->stream_id)
	{
		chunk_header->basic_header.format_type		= RtmpChunkFormat::Type_1;
		chunk_header->basic_header.chunk_stream_id	= message_header->chunk_stream_id;
		chunk_header->type_1.timestamp_delta		= message_header->timestamp - stream->message_header->timestamp;
		chunk_header->type_1.body_size				= message_header->body_size;
		chunk_header->type_1.type_id				= message_header->type_id;

		if(chunk_header->type_1.timestamp_delta >= RTMP_EXTENDED_FORMAT)
		{
			is_extend_type = true;
		}	
	}
    // type_0  or none compress
	else
	{
		chunk_header->basic_header.format_type		= RtmpChunkFormat::Type_0;
		chunk_header->basic_header.chunk_stream_id	= message_header->chunk_stream_id;
		chunk_header->type_0.timestamp				= message_header->timestamp;
		chunk_header->type_0.body_size				= message_header->body_size;
		chunk_header->type_0.type_id				= message_header->type_id;
		chunk_header->type_0.stream_id				= message_header->stream_id;

		if(chunk_header->type_0.timestamp >= RTMP_EXTENDED_FORMAT)
		{
			is_extend_type = true; 
		}
	}

	return chunk_header;
}

//====================================================================================================
// ExportStreamData
//====================================================================================================
std::shared_ptr<std::vector<uint8_t>> RtmpExportChunk::ExportStreamData(std::shared_ptr<RtmpMuxMessageHeader> &message_header, std::shared_ptr<std::vector<uint8_t>> &data)
{
	bool 		is_extend_type = false;
	uint32_t	type3_time = 0;

	if (message_header == nullptr || message_header->chunk_stream_id < 2)
	{
		return nullptr;
	}

	auto stream = GetStream(message_header->chunk_stream_id);

	auto chunk_header = GetChunkHeader(stream, message_header, is_extend_type);

	if (chunk_header->basic_header.format_type == RtmpChunkFormat::Type_0)		type3_time = chunk_header->type_0.timestamp;
	else if (chunk_header->basic_header.format_type == RtmpChunkFormat::Type_1)	type3_time = chunk_header->type_1.timestamp_delta;
	else if (chunk_header->basic_header.format_type == RtmpChunkFormat::Type_2)	type3_time = chunk_header->type_2.timestamp_delta;

	auto chunk_raw_header = MakeChunkRewHeader(chunk_header, is_extend_type);

	auto chunk_raw_data = MakeChunkRawData(_chunk_size, message_header->chunk_stream_id, data, is_extend_type, type3_time);
	
	stream->timestamp_delta = message_header->timestamp - stream->message_header->timestamp;
	stream->message_header = message_header;

	chunk_raw_header->insert(chunk_raw_header->end(), chunk_raw_data->begin(), chunk_raw_data->end());

	return chunk_raw_header;
}
