//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "rtmp_import_chunk.h"

//====================================================================================================
// RtmpImportChunk
//====================================================================================================
RtmpImportChunk::RtmpImportChunk(int chunk_size) 
{
    _stream_map.clear();
    _import_message_queue.clear();
    _chunk_size = chunk_size;
}

//====================================================================================================
// ~RtmpImportChunk
//====================================================================================================
RtmpImportChunk::~RtmpImportChunk()
{
    Destroy();
}

//====================================================================================================
// Destroy
//====================================================================================================
void RtmpImportChunk::Destroy()
{
    _stream_map.clear();
    _import_message_queue.clear();
}

//====================================================================================================
// GetStream
//====================================================================================================
std::shared_ptr<ImportStream> RtmpImportChunk::GetStream(uint32_t chunk_stream_id)
{
    auto item = _stream_map.find(chunk_stream_id);
 
    if (item != _stream_map.end())
    {
        return item->second;
    }
 
    auto stream = std::make_shared<ImportStream>();
 
    _stream_map.insert(std::pair<uint32_t, std::shared_ptr<ImportStream>>(chunk_stream_id, stream));

    return stream;
}

//====================================================================================================
// GetMessageHeader
//====================================================================================================
std::shared_ptr<RtmpMuxMessageHeader> RtmpImportChunk::GetMessageHeader(std::shared_ptr<ImportStream> &stream,
                                                                        std::shared_ptr<RtmpChunkHeader> &chunk_header)
{
    auto message_header = std::make_shared<RtmpMuxMessageHeader>();

    switch (chunk_header->basic_header.format_type)
    {
        case RTMP_CHUNK_TYPE_0:
        {
            message_header->chunk_stream_id = chunk_header->basic_header.chunk_stream_id;
            message_header->timestamp		= chunk_header->type_0.timestamp;
            message_header->body_size		= chunk_header->type_0.body_size;
            message_header->type_id			= chunk_header->type_0.type_id;
            message_header->stream_id		= chunk_header->type_0.stream_id;

		    break;
        }
        case RTMP_CHUNK_TYPE_1:
        {
            message_header->chunk_stream_id = chunk_header->basic_header.chunk_stream_id;
            message_header->timestamp		= stream->message_header->timestamp + chunk_header->type_1.timestamp_delta;
            message_header->body_size		= chunk_header->type_1.body_size;
            message_header->type_id			= chunk_header->type_1.type_id;
            message_header->stream_id		= stream->message_header->stream_id;        //

            break;
        }
        case RTMP_CHUNK_TYPE_2:
        {
            message_header->chunk_stream_id = chunk_header->basic_header.chunk_stream_id;
            message_header->timestamp		= stream->message_header->timestamp + chunk_header->type_2.timestamp_delta;
            message_header->body_size		= stream->message_header->body_size;
            message_header->type_id			= stream->message_header->type_id;
            message_header->stream_id		= stream->message_header->stream_id;

            break;
        }
        case RTMP_CHUNK_TYPE_3:
        {
            message_header->chunk_stream_id = chunk_header->basic_header.chunk_stream_id;
            message_header->timestamp		= stream->message_header->timestamp + stream->timestamp_delta;
            message_header->body_size		= stream->message_header->body_size;
            message_header->type_id			= stream->message_header->type_id;
            message_header->stream_id		= stream->message_header->stream_id;

            break;
        }
        default :
            break;
    }
	
    return message_header;
}

//====================================================================================================
// AppendChunk
//====================================================================================================
bool
RtmpImportChunk::AppendChunk(std::shared_ptr<ImportStream> &stream, uint8_t *chunk, int header_size, int data_size)
{
    int append_size = header_size + data_size;
 
    if (append_size > MAX_MEDIA_PACKET_SIZE)//10M
    {
        LOG_WRITE(("Rtmp Append Data Size ( %d)", append_size));
        return false;
    }
 
	// memory incress
    if ((static_cast<int>(stream->buffer->size()) - stream->data_size) < append_size)
    {
        stream->buffer->resize(
                (stream->buffer->size() * 2) > (stream->buffer->size() + append_size) ? (stream->buffer->size() * 2) : (
                        stream->buffer->size() + append_size));
    }
	 
    memcpy(stream->buffer->data() + stream->data_size, chunk, (size_t) append_size);

    stream->data_size += append_size;
    stream->write_chunk_size += data_size;

    return true;
}



//====================================================================================================
// ImportStream
// - Chunk data insert
//====================================================================================================
int RtmpImportChunk::ImportStreamData(uint8_t *data, int data_size, bool &message_complete)
{
    int chunk_header_size = 0;
    int rest_chunk_size = 0;
    bool is_extend_type = false;

    if (data_size <= 0 || data == nullptr)
    {
        return 0;
    }
 
    auto chunk_header = GetChunkHeader(data, data_size, chunk_header_size, is_extend_type);

    if (chunk_header_size <= 0)
    {
        return chunk_header_size;
    }
 
    auto stream = GetStream(chunk_header->basic_header.chunk_stream_id);

    // Type3 ExtendHeader check
    if (chunk_header->basic_header.format_type == RTMP_CHUNK_TYPE_3)
    {
        is_extend_type = stream->is_extend_type;

        if (is_extend_type)
        {
            chunk_header_size += RTMP_EXTEND_TIMESTAMP_SIZE;

            if (data_size < chunk_header_size)
            {
                LOG_INFO_WRITE(("Type3 Header Extend Size Cut"));
                return 0;
            }
        }
    }

    // ExtendHeader setting
    stream->is_extend_type = is_extend_type;

    // Message Header 얻기
    auto message_header = GetMessageHeader(stream, chunk_header);

    rest_chunk_size = message_header->body_size - stream->write_chunk_size;

    if (rest_chunk_size <= 0)
    {
        LOG_ERROR_WRITE(("Rest Chunk Size Fail - Header(%d) Data(%d) Chunk(%d)", chunk_header_size, rest_chunk_size, _chunk_size));
        return -1;
    }
	 
	// chunk append check(type3)
    if (_chunk_size < rest_chunk_size)
    {
        if (data_size < chunk_header_size + _chunk_size)
        {
		    return 0;
        }
 
        if (!AppendChunk(stream, data, chunk_header_size, _chunk_size))
        {
            LOG_ERROR_WRITE(("AppendChunk Fail - Header(%d) Data(%d) Chunk(%d)", chunk_header_size, rest_chunk_size, _chunk_size));
            return -1;
        }

        // Import stream info update
        message_header->timestamp = stream->message_header->timestamp;
        stream->message_header = message_header;
        message_complete = false;

        return chunk_header_size + _chunk_size;
    }
	 
    if (data_size < chunk_header_size + rest_chunk_size)
    {
        return 0;
    }
	 
    if (!AppendChunk(stream, data, chunk_header_size, rest_chunk_size))
    {
        LOG_ERROR_WRITE(("AppendChunk Fail - Header(%d) Data(%d) Chunk(%d)", chunk_header_size, rest_chunk_size, _chunk_size));
        return -1;
    }
 
	if (CompleteChunkMessage(stream) == false)
	{
		LOG_ERROR_WRITE(("CompleteChunkMessage Fail"));
		return -1;
	}
	
    message_complete = true;
	
    return chunk_header_size + rest_chunk_size;

}

//====================================================================================================
// CompleteChunkMessage
// - data copy
//====================================================================================================
bool RtmpImportChunk::CompleteChunkMessage(std::shared_ptr<ImportStream> &stream)
{
	int chunk_header_size = 0;
	bool is_extend_type = false;

	auto chunk_header = GetChunkHeader(stream->buffer->data(), stream->data_size, chunk_header_size, is_extend_type);
	auto message_header = GetMessageHeader(stream, chunk_header);
	auto chunk_data_size = GetChunkDataRawSize(_chunk_size, message_header->chunk_stream_id, message_header->body_size, is_extend_type);

	if (stream->message_header->chunk_stream_id == 4)
	{
		int i = 0;
		i++;

	}

	// Chunk Stream is not complete skip
	if (stream->data_size < chunk_header_size + chunk_data_size)
	{
		LOG_ERROR_WRITE(("Buffer(%d) RawData(%d)", stream->data_size, chunk_header_size + chunk_data_size));
		return false;
	}

	if (message_header->body_size == 0)
	{
		stream->data_size = 0;
		stream->write_chunk_size = 0;
		stream->timestamp_delta = message_header->timestamp - stream->message_header->timestamp;
		stream->message_header = message_header;
		return true;
	}

	auto message = std::make_shared<ImportMessage>(message_header);

	if (GetChunkData(_chunk_size,
		stream->buffer->data() + chunk_header_size,
		chunk_data_size,
		message_header->body_size,
		message->body->data(),
		is_extend_type) == 0)
	{
		LOG_ERROR_WRITE(("GetChunkData - Header(%d) RawData(%d)", chunk_header_size, chunk_data_size));
		return false;
	}

	stream->data_size = 0;
	stream->write_chunk_size = 0;
	stream->timestamp_delta = message_header->timestamp - stream->message_header->timestamp;
	stream->message_header = message_header;

	_import_message_queue.push_back(message);

	return true;
}

//====================================================================================================
// Import Message
//====================================================================================================
std::shared_ptr<ImportMessage> RtmpImportChunk::GetMessage()
{
    if (_import_message_queue.empty())
    {
        return nullptr;
    }

    auto message = _import_message_queue.front();

    _import_message_queue.pop_front();

    return message;
}
