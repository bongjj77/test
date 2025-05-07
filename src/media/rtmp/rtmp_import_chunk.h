#pragma once
#include "rtmp_mux_util.h"
#include <deque>
#include <map>
#include <memory>
#include <vector>

//====================================================================================================
// ImportStream
//====================================================================================================
struct ImportStream {
public:
  ImportStream()
      : header(std::make_shared<RtmpMuxMsgHeader>()), timestampDelta(0), isEextend(false), writeChunkDataSize(0),
        writeBufferSize(0), buffer(std::make_shared<std::vector<uint8_t>>(1024 * 1024)) {}

  std::shared_ptr<RtmpMuxMsgHeader> header;
  uint32_t timestampDelta;
  bool isEextend;
  int writeChunkDataSize;
  int writeBufferSize;
  std::shared_ptr<std::vector<uint8_t>> buffer;
};

//====================================================================================================
// ImportMsg
//====================================================================================================
struct ImportMsg {
public:
  ImportMsg(const std::shared_ptr<RtmpMuxMsgHeader> &header_, std::shared_ptr<std::vector<uint8_t>> body_)
      : header(header_), body(std::move(body_)) {}

  std::shared_ptr<RtmpMuxMsgHeader> header;
  std::shared_ptr<std::vector<uint8_t>> body;
};

//====================================================================================================
// RtmpImportChunk
//====================================================================================================
class RtmpImportChunk : public RtmpMuxUtil {
public:
  explicit RtmpImportChunk(int chunkSize);
  ~RtmpImportChunk() override = default;

  void Destroy();
  std::pair<int, bool> ImportStreamData(const uint8_t *data, int dataSize);
  std::shared_ptr<ImportMsg> GetMessage();
  void SetChunkSize(int chunkSize) { _chunkSize = chunkSize; }

private:
  std::shared_ptr<ImportStream> GetStream(uint32_t chunkStreamId);
  std::shared_ptr<RtmpMuxMsgHeader> GetMessageHeader(const std::shared_ptr<ImportStream> &stream,
                                                     const std::shared_ptr<Rtmp::ChunkHeader> &chunkHeader);
  bool AppendChunk(const std::shared_ptr<ImportStream> &stream, const uint8_t *chunk, int headerSize, int dataSize);
  bool CompletedChunkMessage(const std::shared_ptr<ImportStream> &stream);

private:
  std::map<uint32_t, std::shared_ptr<ImportStream>> _streamMap;
  std::deque<std::shared_ptr<ImportMsg>> _importMessageQueue;
  int _chunkSize;
};