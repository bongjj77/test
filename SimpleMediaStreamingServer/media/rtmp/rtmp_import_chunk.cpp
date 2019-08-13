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
        case RTMP_CHUNK_BASIC_FORMAT_TYPE0:
        {
            message_header->chunk_stream_id = chunk_header->basic_header.chunk_stream_id;
            message_header->timestamp = chunk_header->type_0.timestamp;
            message_header->body_size = chunk_header->type_0.body_size;
            message_header->type_id = chunk_header->type_0.type_id;
            message_header->stream_id = chunk_header->type_0.stream_id;
            break;
        }
        case RTMP_CHUNK_BASIC_FORMAT_TYPE1:
        {
            message_header->chunk_stream_id = chunk_header->basic_header.chunk_stream_id;
            message_header->timestamp = stream->message_header->timestamp + chunk_header->type_1.timestamp_delta;
            message_header->body_size = chunk_header->type_1.body_size;
            message_header->type_id = chunk_header->type_1.type_id;
            message_header->stream_id = stream->message_header->stream_id;        //
            break;
        }
        case RTMP_CHUNK_BASIC_FORMAT_TYPE2:
        {
            message_header->chunk_stream_id = chunk_header->basic_header.chunk_stream_id;
            message_header->timestamp = stream->message_header->timestamp + chunk_header->type_2.timestamp_delta;
            message_header->body_size = stream->message_header->body_size;
            message_header->type_id = stream->message_header->type_id;
            message_header->stream_id = stream->message_header->stream_id;
            break;
        }
        case RTMP_CHUNK_BASIC_FORMAT_TYPE3:
        {
            message_header->chunk_stream_id = chunk_header->basic_header.chunk_stream_id;
            message_header->timestamp = stream->message_header->timestamp + stream->timestamp_delta;
            message_header->body_size = stream->message_header->body_size;
            message_header->type_id = stream->message_header->type_id;
            message_header->stream_id = stream->message_header->stream_id;
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
// CompleteChunkMessage
// - data copy
//====================================================================================================
int RtmpImportChunk::CompleteChunkMessage(std::shared_ptr<ImportStream> &stream, int chunk_size)
{
    int chunk_header_size = 0;
    int chunk_data_raw_size = 0;
    bool extend_type = false;
 
    auto chunk_header = GetChunkHeader(stream->buffer->data(), stream->data_size, chunk_header_size, extend_type);
 
    auto message_header = GetMessageHeader(stream, chunk_header);
 
    chunk_data_raw_size = GetChunkDataRawSize(chunk_size, message_header->chunk_stream_id, message_header->body_size,
                                              extend_type);

    // Chunk Stream is not complete skip
    if (stream->data_size < chunk_header_size + chunk_data_raw_size)
    {
        LOG_WRITE(("ERROR : Buffer(%d) RawData(%d)", stream->data_size, chunk_header_size + chunk_data_raw_size));
        return 0;
    }
 
    if (message_header->body_size == 0)
    {
        stream->data_size = 0;
        stream->write_chunk_size = 0;
        return 0;
    }
	 
    auto message = std::make_shared<ImportMessage>(message_header);
	 
    if (GetChunkData(chunk_size, stream->buffer->data() + chunk_header_size, chunk_data_raw_size,
                     message_header->body_size, message->body->data(), extend_type) == 0)
    {
		LOG_WRITE(("ERROR : GetChunkData - Header(%d) RawData(%d)", chunk_header_size, chunk_data_raw_size));
        return 0;
    }
	 
    stream->data_size = 0;
    stream->write_chunk_size = 0;
 
    _import_message_queue.push_back(message);

    return message_header->body_size;
}

//====================================================================================================
// ImportStream
// - Chunk data insert
//====================================================================================================
int RtmpImportChunk::ImportStreamData(uint8_t *data, int data_size, bool &message_complete)
{
    int chunk_header_size = 0;
    int rest_chunk_size = 0;
    bool extend_type = false;

    if (data_size <= 0 || data == nullptr)
    {
        return 0;
    }
 
    auto chunk_header = GetChunkHeader(data, data_size, chunk_header_size, extend_type);

    if (chunk_header_size <= 0)
    {
        return chunk_header_size;
    }
 
    auto stream = GetStream(chunk_header->basic_header.chunk_stream_id);

    // Type3 ExtendHeader check
    if (chunk_header->basic_header.format_type == RTMP_CHUNK_BASIC_FORMAT_TYPE3)
    {
        extend_type = stream->extend_type;

        if (extend_type)
        {
            chunk_header_size += RTMP_EXTEND_TIMESTAMP_SIZE;

            if (data_size < chunk_header_size)
            {
                LOG_WRITE(("INFO : Type3 Header Extend Size Cut"));
                return 0;
            }
        }
    }

    // ExtendHeader setting
    stream->extend_type = extend_type;

    // Message Header 얻기
    auto message_header = GetMessageHeader(stream, chunk_header);

    rest_chunk_size = message_header->body_size - stream->write_chunk_size;

    if (rest_chunk_size <= 0)
    {
        LOG_WRITE(("ERROR : Rest Chunk Size Fail - Header(%d) Data(%d) Chunk(%d)", 
					chunk_header_size, rest_chunk_size, _chunk_size));
        return -1;
    }
	 
    if (_chunk_size < rest_chunk_size)
    {
        if (data_size < chunk_header_size + _chunk_size)
        {
            return 0;
        }
 
        if (!AppendChunk(stream, data, chunk_header_size, _chunk_size))
        {
            LOG_WRITE(("ERROR : AppendChunk Fail - Header(%d) Data(%d) Chunk(%d)", 
						chunk_header_size, rest_chunk_size, _chunk_size));
            return -1;
        }

        // Import stream info update
        message_header->timestamp -= stream->timestamp_delta;
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
        LOG_WRITE(("ERROR : AppendChunk Fail - Header(%d) Data(%d) Chunk(%d)", 
					chunk_header_size, rest_chunk_size, _chunk_size));
        return -1;
    }
 
    CompleteChunkMessage(stream, _chunk_size);
	 
    stream->timestamp_delta = message_header->timestamp - stream->message_header->timestamp;
    stream->message_header = message_header;
    message_complete = true;

    return chunk_header_size + rest_chunk_size;

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
