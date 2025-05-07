#pragma once
#include "media/rtmp/amf_document.h"
#include "media/rtmp/rtmp_export_chunk.h"
#include "media/rtmp/rtmp_handshake.h"
#include "media/rtmp/rtmp_import_chunk.h"
#include "media/rtmp/rtmp_media_parser.h"
#include <functional>
#include <map>
#include <memory>
#include <vector>

class PlayerStreamEvent {
public:
  virtual ~PlayerStreamEvent() = default;

  virtual bool StreamSendData(const std::shared_ptr<std::vector<uint8_t>> &data) = 0;
  virtual bool OnStreamStart(const std::string &streamPath) = 0;
  virtual std::shared_ptr<Rtmp::MediaInfo> OnStreamPlay(const std::string &streamPath) = 0;
};

//====================================================================================================
// Rtmp Stream
//====================================================================================================
class PlayerStream {
public:
  PlayerStream(const std::shared_ptr<PlayerStreamEvent> &event) : _event(std::move(event)) {}
  virtual ~PlayerStream() = default;

public:
  int RecvHandler(const std::shared_ptr<std::vector<uint8_t>> &data);
  const std::string &GetStreamPath() { return _streamPath; }
  uint32_t GetLastVideoTimestamp() { return _lastVideoTimestamp; }
  uint32_t GetLastAudioTimestamp() { return _lastAudioTimestamp; }

  bool SendVideo(const std::shared_ptr<Rtmp::Frame> &frame);
  bool SendAudio(const std::shared_ptr<Rtmp::Frame> &frame);

protected:
  int32_t RecvHandshake(const std::shared_ptr<const std::vector<uint8_t>> &data);
  int32_t RecvChunk(const std::shared_ptr<const std::vector<uint8_t>> &data);

  bool SendHandshake(const std::shared_ptr<const std::vector<uint8_t>> &data);

  bool RecvChunkMsg();
  bool OnSetChunkSize(const std::shared_ptr<ImportMsg> &msg);
  void OnWindowAckSize(const std::shared_ptr<ImportMsg> &msg);
  void OnAmfCmdMsg(const std::shared_ptr<ImportMsg> &msg);

  void OnAmfConnect(const std::shared_ptr<RtmpMuxMsgHeader> &header, const std::shared_ptr<AmfDoc> &doc,
                    double transacrtionId);
  void OnAmfCreateStream(const std::shared_ptr<RtmpMuxMsgHeader> &header, const std::shared_ptr<AmfDoc> &doc,
                         double transacrtionId);
  void OnAmfDeleteStream(const std::shared_ptr<RtmpMuxMsgHeader> &header, const std::shared_ptr<AmfDoc> &doc,
                         double transacrtionId);

  bool OnAmfPlay(const std::shared_ptr<RtmpMuxMsgHeader> &header, const std::shared_ptr<AmfDoc> &doc);

  bool SendMsg(const std::shared_ptr<RtmpMuxMsgHeader> header, const std::shared_ptr<std::vector<uint8_t>> &data);
  bool SendUserControlMsg(const uint16_t msg, const std::shared_ptr<std::vector<uint8_t>> &data);

  bool SendWindowAckSize();
  bool SendSetPeerBandwidth();
  bool SendStreamBegin();
  bool SendAckSize();

  bool SendAmfCmd(std::shared_ptr<RtmpMuxMsgHeader> header, const std::shared_ptr<AmfDoc> &doc);
  bool SendAmfConnectResult(uint32_t chunkStreamId, double transacrtionId, double objectEncoding);
  bool SendAmfCreateStreamResult(uint32_t chunkStreamId, double transacrtionId);
  bool SendAmfOnStatus(uint32_t chunkStreamId, uint32_t streamId, const char *level, const char *code,
                       const char *description, double client_id);

  // only player
  bool SendMetaData(const std::shared_ptr<AmfDoc> &doc);
  bool SendVideoSequenceData(std::shared_ptr<std::vector<uint8_t>> seqHeader);
  bool SendAudioSequenceData(std::shared_ptr<std::vector<uint8_t>> seqHeader);

private:
  std::string _streamPath;
  std::string _streamApp;
  std::string _streamKey;

  Rtmp::HandshakeState _handshakeState = Rtmp::HandshakeState::Ready;
  std::unique_ptr<RtmpImportChunk> _imortChunk = std::make_unique<RtmpImportChunk>(Rtmp::DefaultChunkSize);
  std::unique_ptr<RtmpExportChunk> _exportChunk = std::make_unique<RtmpExportChunk>(false, Rtmp::DefaultChunkSize);
  std::shared_ptr<Rtmp::MediaInfo> _mediaInfo = std::make_shared<Rtmp::MediaInfo>();
  uint32_t _streamId = 0;
  uint32_t _peerBandwidth = Rtmp::DefaultPeerBandWidth;
  uint32_t _ackSize = Rtmp::DefaultAckSize / 2;
  uint32_t _ackTraffic = 0;
  double _clientId = 12345.0;
  int _chunkStreamId = 0;

  uint64_t _lastVideoTimestamp = 0;
  uint64_t _lastAudioTimestamp = 0;
  uint8_t _audioControlByte = 0; // only player
  bool _isPlayStart = false;

  std::weak_ptr<PlayerStreamEvent> _event;
};
