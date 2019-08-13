//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include <memory>
#include <map>
#include "rtmp_mux_util.h"

//====================================================================================================
// ExportStream
//====================================================================================================
struct ExportStream
{
public :
	ExportStream()
	{
		message_header 	= std::make_shared<RtmpMuxMessageHeader>();
		timestamp_delta = 0;
	}
public :
	std::shared_ptr<RtmpMuxMessageHeader>	message_header;
	uint32_t				    			timestamp_delta;
};


//====================================================================================================
// RtmpExportChunk
//====================================================================================================
class RtmpExportChunk : public RtmpMuxUtil
{
public:
	RtmpExportChunk(bool compress_header, int chunk_size);
	~RtmpExportChunk() override;

public:
	std::shared_ptr<std::vector<uint8_t>> ExportStreamData(std::shared_ptr<RtmpMuxMessageHeader> &message_header, std::shared_ptr<std::vector<uint8_t>> &data);

private:
	void Destroy();
	std::shared_ptr<ExportStream> GetStream(uint32_t chunk_stream_id);
    std::shared_ptr<RtmpChunkHeader> GetChunkHeader(std::shared_ptr<ExportStream> &stream, std::shared_ptr<RtmpMuxMessageHeader> &message_header, bool & extend_type);

private:
	std::map<uint32_t, std::shared_ptr<ExportStream>>	_stream_map;
	bool _compress_header;
	int  _chunk_size;
};




