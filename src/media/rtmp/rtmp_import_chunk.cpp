#include "rtmp_import_chunk.h"

//====================================================================================================
// RtmpImportChunk
//====================================================================================================
RtmpImportChunk::RtmpImportChunk(int chunkSize) : _chunkSize(chunkSize) {}

//====================================================================================================
// Destroy
//====================================================================================================
void RtmpImportChunk::Destroy() {
  _streamMap.clear();
  _importMessageQueue.clear();
}

//====================================================================================================
// GetStream
//====================================================================================================
std::shared_ptr<ImportStream> RtmpImportChunk::GetStream(uint32_t chunkStreamId) {
  if (auto it = _streamMap.find(chunkStreamId); it != _streamMap.end()) {
    return it->second;
  }

  auto stream = std::make_shared<ImportStream>();
  _streamMap[chunkStreamId] = stream;
  return stream;
}

//====================================================================================================
// GetMessageHeader
//====================================================================================================
std::shared_ptr<RtmpMuxMsgHeader>
RtmpImportChunk::GetMessageHeader(const std::shared_ptr<ImportStream> &stream,
                                  const std::shared_ptr<Rtmp::ChunkHeader> &chunkHeader) {
  auto header = std::make_shared<RtmpMuxMsgHeader>();

  switch (chunkHeader->basicHeader.formatType) {
  case Rtmp::ChunkFormat::Type_0:
    header->chunkStreamId = chunkHeader->basicHeader.chunkStreamId;
    header->timestamp = chunkHeader->type_0.timestamp;
    header->bodySize = chunkHeader->type_0.bodySize;
    header->typeId = chunkHeader->type_0.typeId;
    header->streamId = chunkHeader->type_0.streamId;
    break;
  case Rtmp::ChunkFormat::Type_1:
    header->chunkStreamId = chunkHeader->basicHeader.chunkStreamId;
    header->timestamp = stream->header->timestamp + chunkHeader->type_1.timestampDelta;
    header->bodySize = chunkHeader->type_1.bodySize;
    header->typeId = chunkHeader->type_1.typeId;
    header->streamId = stream->header->streamId;
    break;
  case Rtmp::ChunkFormat::Type_2:
    header->chunkStreamId = chunkHeader->basicHeader.chunkStreamId;
    header->timestamp = stream->header->timestamp + chunkHeader->type_2.timestampDelta;
    header->bodySize = stream->header->bodySize;
    header->typeId = stream->header->typeId;
    header->streamId = stream->header->streamId;
    break;
  case Rtmp::ChunkFormat::Type_3:
    header->chunkStreamId = chunkHeader->basicHeader.chunkStreamId;
    header->timestamp = stream->header->timestamp + stream->timestampDelta;
    header->bodySize = stream->header->bodySize;
    header->typeId = stream->header->typeId;
    header->streamId = stream->header->streamId;
    break;
  default:
    break;
  }

  return header;
}

//====================================================================================================
// AppendChunk
//====================================================================================================
bool RtmpImportChunk::AppendChunk(const std::shared_ptr<ImportStream> &stream, const uint8_t *chunk, int headerSize,
                                  int dataSize) {
  int appendSize = (stream->writeChunkDataSize == 0) ? (headerSize + dataSize) : dataSize;

  if (appendSize > Rtmp::MaxPacketSize) {
    LOG_INFO("Rtmp Append Data Size ( %d)", appendSize);
    return false;
  }

  if ((static_cast<int>(stream->buffer->size()) - stream->writeBufferSize) < appendSize) {
    stream->buffer->resize((stream->buffer->size() * 2) > (stream->buffer->size() + appendSize)
                               ? (stream->buffer->size() * 2)
                               : (stream->buffer->size() + appendSize));
  }

  if (stream->writeBufferSize == 0) {
    std::memcpy(stream->buffer->data() + stream->writeBufferSize, chunk, static_cast<size_t>(appendSize));
  } else {
    std::memcpy(stream->buffer->data() + stream->writeBufferSize, chunk + headerSize, static_cast<size_t>(appendSize));
  }

  stream->writeBufferSize += appendSize;
  stream->writeChunkDataSize += dataSize;

  return true;
}

//====================================================================================================
// ImportStreamData
//====================================================================================================
std::pair<int, bool> RtmpImportChunk::ImportStreamData(const uint8_t *data, int dataSize) {
  if (dataSize <= 0 || data == nullptr) {
    return {0, false};
  }

  auto [chunkHeader, chunkHeaderSize, isEextend] = GetChunkHeader(data, dataSize);

  if (chunkHeaderSize <= 0) {
    return {chunkHeaderSize, false};
  }

  auto stream = GetStream(chunkHeader->basicHeader.chunkStreamId);

  if (chunkHeader->basicHeader.formatType == Rtmp::ChunkFormat::Type_3) {
    isEextend = stream->isEextend;

    if (isEextend) {
      chunkHeaderSize += Rtmp::ExtendTimestampSize;

      if (dataSize < chunkHeaderSize) {
        LOG_INFO("Type3 Header Extend Size Cut");
        return {0, false};
      }
    }
  }

  stream->isEextend = isEextend;

  auto header = GetMessageHeader(stream, chunkHeader);

  auto restChunkSize = static_cast<int>(header->bodySize) - stream->writeChunkDataSize;

  if (restChunkSize <= 0) {
    LOG_ERROR("Rest Chunk Size Fail - Header(%d) Data(%d) Chunk(%d)", chunkHeaderSize, restChunkSize, _chunkSize);
    return {-1, false};
  }

  if (_chunkSize < restChunkSize) {
    if (dataSize < chunkHeaderSize + _chunkSize) {
      return {0, false};
    }

    if (!AppendChunk(stream, data, chunkHeaderSize, _chunkSize)) {
      LOG_ERROR("AppendChunk Fail - Header(%d) Data(%d) Chunk(%d)", chunkHeaderSize, restChunkSize, _chunkSize);
      return {-1, false};
    }

    header->timestamp = stream->header->timestamp;
    stream->header = header;

    return {chunkHeaderSize + _chunkSize, false};
  }

  if (dataSize < chunkHeaderSize + restChunkSize) {
    return {0, false};
  }

  if (!AppendChunk(stream, data, chunkHeaderSize, restChunkSize)) {
    LOG_ERROR("AppendChunk Fail - Header(%d) Data(%d) Chunk(%d)", chunkHeaderSize, restChunkSize, _chunkSize);
    return {-1, false};
  }

  if (!CompletedChunkMessage(stream)) {
    LOG_ERROR("CompletedChunkMessage Fail");
    return {-1, false};
  }

  return {chunkHeaderSize + restChunkSize, true};
}

//====================================================================================================
// CompletedChunkMessage
//====================================================================================================
bool RtmpImportChunk::CompletedChunkMessage(const std::shared_ptr<ImportStream> &stream) {
  auto [chunkHeader, chunkHeaderSize, isEextend] = GetChunkHeader(stream->buffer->data(), stream->writeBufferSize);
  auto chunkDataSize = stream->writeChunkDataSize;

  if (stream->writeBufferSize < chunkHeaderSize + chunkDataSize) {
    LOG_ERROR("Buffer(%d) RawData(%d)", stream->writeBufferSize, chunkHeaderSize + chunkDataSize);
    return false;
  }

  auto header = GetMessageHeader(stream, chunkHeader);

  if (header->bodySize != 0) {
    auto chunkData = std::make_shared<std::vector<uint8_t>>(stream->buffer->begin() + chunkHeaderSize,
                                                            stream->buffer->begin() + chunkHeaderSize + chunkDataSize);
    auto msg = std::make_shared<ImportMsg>(header, chunkData);

    _importMessageQueue.push_back(msg);
  }

  stream->writeBufferSize = 0;
  stream->writeChunkDataSize = 0;
  stream->timestampDelta = header->timestamp - stream->header->timestamp;
  stream->header = header;

  return true;
}

//====================================================================================================
// GetMessage
//====================================================================================================
std::shared_ptr<ImportMsg> RtmpImportChunk::GetMessage() {
  if (_importMessageQueue.empty()) {
    return nullptr;
  }

  auto msg = _importMessageQueue.front();
  _importMessageQueue.pop_front();

  return msg;
}