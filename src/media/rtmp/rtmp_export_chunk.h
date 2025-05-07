#pragma once
#include "rtmp_mux_util.h"
#include <map>
#include <memory>
#include <vector>

//====================================================================================================
// ExportStream
//====================================================================================================
struct ExportStream {
  ExportStream() : header(std::make_shared<RtmpMuxMsgHeader>()), timestampDelta(0) {}

  std::shared_ptr<RtmpMuxMsgHeader> header;
  uint32_t timestampDelta;
};

//====================================================================================================
// RtmpExportChunk
//====================================================================================================
class RtmpExportChunk : public RtmpMuxUtil {
public:
  RtmpExportChunk(bool compressHeader, int chunkSize);
  ~RtmpExportChunk() override = default;

  std::shared_ptr<std::vector<uint8_t>> ExportStreamData(const std::shared_ptr<RtmpMuxMsgHeader> &header,
                                                         const std::shared_ptr<std::vector<uint8_t>> &data);
  int GetChunkSize() { return _chunkSize; }

private:
  void Destroy();
  std::shared_ptr<ExportStream> GetStream(uint32_t chunkStreamId);
  std::pair<std::shared_ptr<Rtmp::ChunkHeader>, bool> GetChunkHeader(const std::shared_ptr<ExportStream> &stream,
                                                                     const std::shared_ptr<RtmpMuxMsgHeader> &header);

private:
  std::map<uint32_t, std::shared_ptr<ExportStream>> _streamMap;
  bool _compressHeader;
  int _chunkSize;
};