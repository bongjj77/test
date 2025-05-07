#include "rtmp_export_chunk.h"

//====================================================================================================
// Constructor
//====================================================================================================
RtmpExportChunk::RtmpExportChunk(bool compressHeader, int chunkSize)
    : _compressHeader(compressHeader), _chunkSize(chunkSize) {}

//====================================================================================================
// Destroy
//====================================================================================================
void RtmpExportChunk::Destroy() { _streamMap.clear(); }

//====================================================================================================
// GetStream
//====================================================================================================
std::shared_ptr<ExportStream> RtmpExportChunk::GetStream(uint32_t chunkStreamId) {
  if (auto it = _streamMap.find(chunkStreamId); it != _streamMap.end()) {
    return it->second;
  }

  auto stream = std::make_shared<ExportStream>();
  _streamMap[chunkStreamId] = stream;
  return stream;
}

//====================================================================================================
// GetChunkHeader
//====================================================================================================
std::pair<std::shared_ptr<Rtmp::ChunkHeader>, bool>
RtmpExportChunk::GetChunkHeader(const std::shared_ptr<ExportStream> &stream,
                                const std::shared_ptr<RtmpMuxMsgHeader> &header) {
  auto chunkHeader = std::make_shared<Rtmp::ChunkHeader>();
  bool isExtended = false;

  auto &prevHeader = stream->header;
  uint32_t timestampDelta = header->timestamp - prevHeader->timestamp;

  if (_compressHeader && stream->header->chunkStreamId == header->chunkStreamId) {
    if (stream->header->bodySize == header->bodySize && stream->header->streamId == header->streamId &&
        stream->header->typeId == header->typeId) {
      chunkHeader->basicHeader.formatType = Rtmp::ChunkFormat::Type_2;
      chunkHeader->basicHeader.chunkStreamId = header->chunkStreamId;
      chunkHeader->type_2.timestampDelta = timestampDelta;

      if (timestampDelta >= Rtmp::ExtendFormat) {
        isExtended = true;
      }
    } else {
      chunkHeader->basicHeader.formatType = Rtmp::ChunkFormat::Type_1;
      chunkHeader->basicHeader.chunkStreamId = header->chunkStreamId;
      chunkHeader->type_1.timestampDelta = timestampDelta;
      chunkHeader->type_1.bodySize = header->bodySize;
      chunkHeader->type_1.typeId = header->typeId;

      if (timestampDelta >= Rtmp::ExtendFormat) {
        isExtended = true;
      }
    }
  } else {
    chunkHeader->basicHeader.formatType = Rtmp::ChunkFormat::Type_0;
    chunkHeader->basicHeader.chunkStreamId = header->chunkStreamId;
    chunkHeader->type_0.timestamp = header->timestamp;
    chunkHeader->type_0.bodySize = header->bodySize;
    chunkHeader->type_0.typeId = header->typeId;
    chunkHeader->type_0.streamId = header->streamId;

    if (chunkHeader->type_0.timestamp >= Rtmp::ExtendFormat) {
      isExtended = true;
    }
  }

  return {chunkHeader, isExtended};
}

//====================================================================================================
// ExportStreamData
//====================================================================================================
std::shared_ptr<std::vector<uint8_t>>
RtmpExportChunk::ExportStreamData(const std::shared_ptr<RtmpMuxMsgHeader> &header,
                                  const std::shared_ptr<std::vector<uint8_t>> &data) {
  if (!header || header->chunkStreamId < 2) {
    return nullptr;
  }

  auto stream = GetStream(header->chunkStreamId);
  auto [chunkHeader, isExtended] = GetChunkHeader(stream, header);

  uint32_t type3Time = 0;
  switch (chunkHeader->basicHeader.formatType) {
  case Rtmp::ChunkFormat::Type_0:
    type3Time = chunkHeader->type_0.timestamp;
    break;
  case Rtmp::ChunkFormat::Type_1:
    type3Time = chunkHeader->type_1.timestampDelta;
    break;
  case Rtmp::ChunkFormat::Type_2:
    type3Time = chunkHeader->type_2.timestampDelta;
    break;
  default:
    break;
  }

  auto chunkRawHeader = MakeChunkRawHeader(chunkHeader, isExtended);
  auto chunkRawData = MakeChunkRawData(_chunkSize, header->chunkStreamId, data, isExtended, type3Time);

  stream->timestampDelta = header->timestamp - stream->header->timestamp;
  stream->header = header;

  chunkRawHeader->insert(chunkRawHeader->end(), chunkRawData->begin(), chunkRawData->end());
  return chunkRawHeader;
}