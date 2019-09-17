//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include <memory>
#include <map>
#include <deque>
#include "rtmp_mux_util.h"

//====================================================================================================
// ImportStream
//====================================================================================================
struct ImportStream
{
public :
	ImportStream()
	{
		message_header = std::make_shared<RtmpMuxMessageHeader>();
		is_extend_type = false;
		write_buffer_size = 0;
		write_chunk_data_size = 0;
		buffer = std::make_shared<std::vector<uint8_t>>(1024 * 1024);
	}

public :
	std::shared_ptr<RtmpMuxMessageHeader> message_header;
	uint32_t	timestamp_delta;
	bool		is_extend_type;
	int			write_chunk_data_size;
	int 		write_buffer_size;
	std::shared_ptr<std::vector<uint8_t>> buffer;
};

//====================================================================================================
// ImportMessage
//====================================================================================================
struct ImportMessage
{
public :
	ImportMessage(std::shared_ptr<RtmpMuxMessageHeader>	&header_, std::shared_ptr<std::vector<uint8_t>> body_)
	{
		message_header = header_;
		body = body_;
	}

public :
	std::shared_ptr<RtmpMuxMessageHeader>	message_header;
	std::shared_ptr<std::vector<uint8_t>> 	body;
};

//====================================================================================================
// RtmpImportChunk
//====================================================================================================
class RtmpImportChunk : public RtmpMuxUtil
{
public:
	RtmpImportChunk(int chunk_size);
	~RtmpImportChunk() override;

public:
	void Destroy();
	int ImportStreamData(uint8_t *data, int data_size, bool & is_message_complete);
	std::shared_ptr<ImportMessage> GetMessage();
	void SetChunkSize(int chunk_size) { _chunk_size = chunk_size; }
private:
	std::shared_ptr<ImportStream> GetStream(uint32_t chunk_stream_id);
	std::shared_ptr<RtmpMuxMessageHeader> GetMessageHeader(std::shared_ptr<ImportStream> &stream, std::shared_ptr<RtmpChunkHeader> &chunk_header);
	bool AppendChunk(std::shared_ptr<ImportStream> &stream, uint8_t *chunk, int header_size, int data_size);
	bool CompletedChunkMessage(std::shared_ptr<ImportStream> &stream);

private:
	std::map<uint32_t, std::shared_ptr<ImportStream>> _stream_map;
	std::deque<std::shared_ptr<ImportMessage>> _import_message_queue;
	int _chunk_size;
};




