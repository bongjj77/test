#include "player_stream.h"
#include "common/common_header.h"
#include <string>
#include <utility>

/*
# Player 절차
  - handshake
  - connect
  - createStream
  - play
  - meta/video/audio stream
  - ...
  - closeStream/releaseStream/close

*/

//====================================================================================================
//  OnDataReceived
//====================================================================================================
int PlayerStream::RecvHandler(const std::shared_ptr<std::vector<uint8_t>> &data) {
  if (data->size() > Rtmp::MaxPacketSize) {
    LOG_ERROR("player - recv data size fail - stream(%s) size(%d)", _streamPath.c_str(), data->size());
    return -1;
  }

  int procSize = (_handshakeState != Rtmp::HandshakeState::Complete) ? RecvHandshake(data) : RecvChunk(data);

  if (procSize < 0) {
    LOG_ERROR("player - process size fail - stream(%s) size(%d)", _streamPath.c_str(), procSize);
    return -1;
  }

  return procSize;
}

//====================================================================================================
// Handshake
//====================================================================================================
int PlayerStream::RecvHandshake(const std::shared_ptr<const std::vector<uint8_t>> &data) {
  int procSize = 0;

  if (_handshakeState == Rtmp::HandshakeState::Ready) {
    procSize = 1 + Rtmp::HandshakePacketSize; // c0 + c1
  } else if (_handshakeState == Rtmp::HandshakeState::S2) {
    procSize = (Rtmp::HandshakePacketSize); // c2
  } else {
    LOG_ERROR("Rtmp handshake state Fail - state(%d)", static_cast<int32_t>(_handshakeState));
    return -1;
  }

  // Process Data Size Check
  if (static_cast<int>(data->size()) < procSize) {
    return 0;
  }

  // c0+c1 / s0+s1+s2
  if (_handshakeState == Rtmp::HandshakeState::Ready) {
    if ((*data)[0] != Rtmp::HandshakeVersion) {
      LOG_WRITE("player - handshake version fail - version(%d:%d)", (*data)[0], Rtmp::HandshakeVersion);
      return -1;
    }
    _handshakeState = Rtmp::HandshakeState::C0;

    // S0+S1+S2
    if (!SendHandshake(data)) {
      return -1;
    }

    return procSize;
  }

  // C2 check none( C2=S2)
  _handshakeState = Rtmp::HandshakeState::C2;

  // chunk가 같이 들어오는 경우 처리
  if (procSize < static_cast<int>(data->size())) {
    int chunkProcSize = RecvChunk(std::make_shared<std::vector<uint8_t>>(data->begin() + procSize, data->end()));

    if (chunkProcSize < 0) {
      return -1;
    }

    procSize += chunkProcSize;
  }

  _handshakeState = Rtmp::HandshakeState::Complete;

  return procSize;
}

//====================================================================================================
// SendHandshake
// s0+s1+s2
//====================================================================================================
bool PlayerStream::SendHandshake(const std::shared_ptr<const std::vector<uint8_t>> &data) {
  _handshakeState = Rtmp::HandshakeState::C1;

  // S0 S1 S2 Send
  auto event = _event.lock();
  if (!event) {
    return false;
  }

  if (!event->StreamSendData(RtmpHandshake::MakeS0_S1_S2(data->data() + 1))) {
    LOG_ERROR("player - handshake s0+s1+s2 send fail");
    return false;
  }

  _handshakeState = Rtmp::HandshakeState::S2;

  return true;
}

//====================================================================================================
// RecvChunk
//====================================================================================================
int32_t PlayerStream::RecvChunk(const std::shared_ptr<const std::vector<uint8_t>> &data) {
  int procSize = 0;

  while (procSize < static_cast<int>(data->size())) {
    auto [size, isComplete] = _imortChunk->ImportStreamData(data->data() + procSize, data->size() - procSize);

    if (size == 0) {
      break;
    } else if (size < 0) {
      LOG_ERROR("player - importStream fail - stream(%s)", _streamPath.c_str());
      return size;
    }

    if (isComplete) {
      if (!RecvChunkMsg()) {
        LOG_ERROR("Rtmp publisher - chunk message fail - stream(%s)", _streamPath.c_str());
        return -1;
      }
    }

    procSize += size;
  }

  // Ack append
  _ackTraffic += procSize;

  if (_ackTraffic > _ackSize) {
    SendAckSize();

    // Init
    _ackTraffic = 0;
  }

  return procSize;
}

//====================================================================================================
// RecvChunkMsg
//====================================================================================================
bool PlayerStream::RecvChunkMsg() {
  while (true) {
    auto msg = _imortChunk->GetMessage();

    if (msg == nullptr || msg->body == nullptr) {
      break;
    }

    if (msg->header->bodySize > Rtmp::MaxPacketSize) {
      LOG_ERROR("player - packet size fail - stream(%s) size(%u:%u)", _streamPath.c_str(), msg->header->bodySize,
                Rtmp::MaxPacketSize);
      return false;
    }

    bool result = true;
    auto type = msg->header->typeId;

    LOG_WRITE("RecvChunkMsg %d", type);

    switch (type) {
    case static_cast<int>(Rtmp::MsgType::SetChunkSize): {
      result = OnSetChunkSize(msg);
      break;
    }
    case static_cast<int>(Rtmp::MsgType::Amf0CmdMsg): {
      OnAmfCmdMsg(msg);
      break;
    }
    case static_cast<int>(Rtmp::MsgType::Ack): {
      // Do nothing
      break;
    }
    case static_cast<int>(Rtmp::MsgType::WindowAckSize): {
      OnWindowAckSize(msg);
      break;
    }
    default: {
      LOG_WARN("player - unknown type - stream(%s) type(%d)", _streamPath.c_str(), msg->header->typeId);
    }
    }

    if (!result) {
      return false;
    }
  }

  return true;
}

//====================================================================================================
// Chunk Message - SetChunkSize
//====================================================================================================
bool PlayerStream::OnSetChunkSize(const std::shared_ptr<ImportMsg> &msg) {
  auto chunkSize = RtmpMuxUtil::ReadInt32(msg->body->data());
  if (chunkSize <= 0) {
    LOG_ERROR("player - chunkSize fail - stream(%s)", _streamPath.c_str());
    return false;
  }

  LOG_INFO("player - set receive chunk - stream(%s) size(%u)", _streamPath.c_str(), chunkSize);
  _imortChunk->SetChunkSize(chunkSize);
  return true;
}
//====================================================================================================
// Chunk Message - WindowAckSize
//====================================================================================================
void PlayerStream::OnWindowAckSize(const std::shared_ptr<ImportMsg> &msg) {
  if (auto ackledgement_size = RtmpMuxUtil::ReadInt32(msg->body->data()); ackledgement_size != 0) {
    _ackSize = ackledgement_size / 2;
    _ackTraffic = 0;
  }
}

//====================================================================================================
// Chunk Message - Amf0CmdMsg
//====================================================================================================
void PlayerStream::OnAmfCmdMsg(const std::shared_ptr<ImportMsg> &msg) {
  auto doc = std::make_shared<AmfDoc>();
  if (doc->Decode(msg->body) == 0) {
    LOG_WRITE("player - amf doc size 0 - stream(%s)", _streamPath.c_str());
    return;
  }

  std::string command;
  if (doc->GetProp(0) != nullptr && doc->GetProp(0)->GetType() == AmfDataType::String) {
    command = doc->GetProp(0)->GetString();
  }

  double transacrtionId = 0.0;
  if (doc->GetProp(1) != nullptr && doc->GetProp(1)->GetType() == AmfDataType::Number) {
    transacrtionId = doc->GetProp(1)->GetNumber();
  }

  if (command.empty()) {
    LOG_WRITE("player - amf message name empty - stream(%s)", _streamPath.c_str());
    return;
  }

  if (command == Rtmp::Cmd::Connect) {
    OnAmfConnect(msg->header, doc, transacrtionId);
  } else if (command == Rtmp::Cmd::CreateStream) {
    OnAmfCreateStream(msg->header, doc, transacrtionId);
  } else if (command == Rtmp::Cmd::ReleaseStream) {
    //
  } else if (command == Rtmp::Cmd::Ping) {
    //
  } else if (command == Rtmp::Cmd::DeleteStream) {
    OnAmfDeleteStream(msg->header, doc, transacrtionId);
  } else if (command == Rtmp::Cmd::Play) {
    OnAmfPlay(msg->header, doc);
  } else {
    LOG_WARN("player - unknown amf0 command message - stream(%s) "
             "message(%s:%.1f)",
             _streamPath.c_str(), command.c_str(), transacrtionId);
    return;
  }
}

//====================================================================================================
// OnAmfConnect
//====================================================================================================
void PlayerStream::OnAmfConnect(const std::shared_ptr<RtmpMuxMsgHeader> &header, const std::shared_ptr<AmfDoc> &doc,
                                double transacrtionId) {
  double objectEncoding = 0.0;

  if (doc->GetProp(2) != nullptr && doc->GetProp(2)->GetType() == AmfDataType::Object) {
    auto object = doc->GetProp(2)->GetObject();
    int32_t index;

    // object encoding
    if ((index = object->FindName("objectEncoding")) >= 0 && object->GetType(index) == AmfDataType::Number) {
      objectEncoding = object->GetNumber(index);
    }

    // app
    if ((index = object->FindName("app")) >= 0 && object->GetType(index) == AmfDataType::String) {
      _streamApp = object->GetString(index);
    }
  }

  if (!SendWindowAckSize()) {
    LOG_ERROR("player - send window acknowledgement size fail - stream(%s)", _streamPath.c_str());
    return;
  }

  if (!SendSetPeerBandwidth()) {
    LOG_ERROR("SendSetPeerBandwidth fail - stream(%s)", _streamPath.c_str());
    return;
  }

  if (!SendStreamBegin()) {
    LOG_ERROR("player - send stream begin fail - stream(%s)", _streamPath.c_str());
    return;
  }

  if (!SendAmfConnectResult(header->chunkStreamId, transacrtionId, objectEncoding)) {
    LOG_ERROR("player - send amf connect result fail - stream(%s)", _streamPath.c_str());
    return;
  }
}

//====================================================================================================
// Amf Command - CreateStream
//====================================================================================================
void PlayerStream::OnAmfCreateStream(const std::shared_ptr<RtmpMuxMsgHeader> &header,
                                     const std::shared_ptr<AmfDoc> &doc, double transacrtionId) {
  if (!SendAmfCreateStreamResult(header->chunkStreamId, transacrtionId)) {
    LOG_ERROR("player - send amf create stream result fail - stream(%s)", _streamPath.c_str());
    return;
  }
}

//====================================================================================================
// Chunk Message - OnAmfPlay
//====================================================================================================
bool PlayerStream::OnAmfPlay(const std::shared_ptr<RtmpMuxMsgHeader> &header, const std::shared_ptr<AmfDoc> &doc) {
  const auto chunkId = header->chunkStreamId;
  const int nameIndex = 3;

  // app/stream check
  if ((doc->GetProp(nameIndex) == nullptr || doc->GetProp(nameIndex)->GetType() != AmfDataType::String) ||
      _streamApp.empty()) {
    LOG_ERROR("player - stream name empty");
    SendAmfOnStatus(chunkId, _streamId, "error", "NetStream.Play.BadConnection", "Authentication Failed.", _clientId);
    return false;
  }

  // stream key setting
  _streamKey = doc->GetProp(nameIndex)->GetString();
  _streamPath = _streamApp + "/" + _streamKey;

  // play callback
  auto event = _event.lock();
  if (!event) {
    return false;
  }

  const auto mediaInfo = event->OnStreamPlay(_streamPath);
  if (mediaInfo == nullptr) {
    // Reject
    SendAmfOnStatus(chunkId, _streamId, "error", "NetStream.Play.BadConnection", "Authentication Failed.", _clientId);
    return false;
  }

  _mediaInfo = mediaInfo;

  SendAmfOnStatus(chunkId, _streamId, "status", "NetStream.Play.Reset", "Playing and resetting stream.", _clientId);
  SendAmfOnStatus(chunkId, _streamId, "status", "NetStream.Play.Start", "Started playing stream.", _clientId);

  if (_mediaInfo->metaData && !SendMetaData(_mediaInfo->metaData)) {
    LOG_ERROR("player -  meta data send fail - stream(%s)", _streamPath.c_str());
  }

  if (_mediaInfo->audioSeqHeader && !SendAudioSequenceData(_mediaInfo->audioSeqHeader)) {
    LOG_ERROR("player -  video sequence data send fail - stream(%s)", _streamPath.c_str());
  }

  if (_mediaInfo->videoSeqHeader && !SendVideoSequenceData(_mediaInfo->videoSeqHeader)) {
    LOG_ERROR("player -  audio sequence data send fail - stream(%s)", _streamPath.c_str());
  }

  _isPlayStart = true;

  return true;
}

//====================================================================================================
// Amf Command - Publish
//====================================================================================================
void PlayerStream::OnAmfDeleteStream(const std::shared_ptr<RtmpMuxMsgHeader> &header,
                                     const std::shared_ptr<AmfDoc> &doc, double transacrtionId) {
  LOG_INFO("player - delete stream - stream(%s)", _streamPath.c_str());
  // _delete_stream = true;
  // _stream_interface->OnChunkStreamDelete(_remote, _appName, _app_stream_name,
  // _app_id, _app_stream_id);
}

//====================================================================================================
// SendMsg
//====================================================================================================
bool PlayerStream::SendMsg(std::shared_ptr<RtmpMuxMsgHeader> header,
                           const std::shared_ptr<std::vector<uint8_t>> &data) {
  if (header == nullptr) {
    return false;
  }

  auto exportData = _exportChunk->ExportStreamData(header, data);

  if (exportData == nullptr || exportData->data() == nullptr) {
    return false;
  }

  auto event = _event.lock();
  if (!event) {
    return false;
  }

  return event->StreamSendData(exportData);
}

//====================================================================================================
// SendAmfCmd
//====================================================================================================
bool PlayerStream::SendAmfCmd(std::shared_ptr<RtmpMuxMsgHeader> header, const std::shared_ptr<AmfDoc> &doc) {
  if (header == nullptr) {
    return false;
  }

  auto body = std::make_shared<std::vector<uint8_t>>(2048);

  // body
  uint32_t bodySize = static_cast<uint32_t>(doc->Encode(body->data()));
  if (bodySize == 0) {
    return false;
  }

  header->bodySize = bodySize;
  body->resize(bodySize);

  return SendMsg(header, body);
}

//====================================================================================================
// USendUserControlMessage
//====================================================================================================
bool PlayerStream::SendUserControlMsg(uint16_t msg, const std::shared_ptr<std::vector<uint8_t>> &data) {
  data->insert(data->begin(), 0);
  data->insert(data->begin(), 0);
  RtmpMuxUtil::WriteInt16(data->data(), msg);

  return SendMsg(std::make_shared<RtmpMuxMsgHeader>(static_cast<int>(Rtmp::ChunkStreamType::Urgent), 0,
                                                    static_cast<int>(Rtmp::MsgType::UserControlMsg), 0, data->size()),
                 data);
}

//====================================================================================================
// WindowAckSize
//====================================================================================================
bool PlayerStream::SendWindowAckSize() {
  auto body = std::make_shared<std::vector<uint8_t>>(sizeof(int));
  RtmpMuxUtil::WriteInt32(body->data(), Rtmp::DefaultAckSize);

  return SendMsg(std::make_shared<RtmpMuxMsgHeader>(static_cast<int>(Rtmp::ChunkStreamType::Urgent), 0,
                                                    static_cast<int>(Rtmp::MsgType::WindowAckSize), _streamId,
                                                    body->size()),
                 body);
}

//====================================================================================================
// SendAckSize
//====================================================================================================
bool PlayerStream::SendAckSize() {
  auto body = std::make_shared<std::vector<uint8_t>>(sizeof(int));
  RtmpMuxUtil::WriteInt32(body->data(), _ackTraffic);

  return SendMsg(std::make_shared<RtmpMuxMsgHeader>(static_cast<int>(Rtmp::ChunkStreamType::Urgent), 0,
                                                    static_cast<int>(Rtmp::MsgType::Ack), 0, body->size()),
                 body);
}

//====================================================================================================
// SendSetPeerBandwidth
//====================================================================================================
bool PlayerStream::SendSetPeerBandwidth() {
  auto body = std::make_shared<std::vector<uint8_t>>(5);
  RtmpMuxUtil::WriteInt32(body->data(), _peerBandwidth);
  RtmpMuxUtil::WriteInt8(body->data() + 4, 2);

  return SendMsg(std::make_shared<RtmpMuxMsgHeader>(static_cast<int>(Rtmp::ChunkStreamType::Urgent), 0,
                                                    static_cast<int>(Rtmp::MsgType::SetPeerBandWidth), _streamId,
                                                    body->size()),
                 body);
}

//====================================================================================================
// SendStreamBegin
//====================================================================================================
bool PlayerStream::SendStreamBegin() {
  auto body = std::make_shared<std::vector<uint8_t>>(4);

  RtmpMuxUtil::WriteInt32(body->data(), _streamId);

  return SendUserControlMsg(static_cast<int>(Rtmp::UserControlMsgType::StreamBegin), body);
}

//====================================================================================================
// SendAmfConnectResult
//====================================================================================================
bool PlayerStream::SendAmfConnectResult(uint32_t chunkStreamId, double transacrtionId, double objectEncoding) {

  // properties
  auto propObject = std::make_shared<AmfObject>();
  propObject->AddProp("fmsVer", "FMS/3,5,2,654");
  propObject->AddProp("capabilities", 31.0);
  propObject->AddProp("mode", 1.0);

  // information
  auto array = std::make_shared<AmfArray>();
  array->AddProp("version", "3,5,2,654");

  auto infoObject = std::make_shared<AmfObject>();
  infoObject->AddProp("level", "status");
  infoObject->AddProp("code", "NetConnection.Connect.Success");
  infoObject->AddProp("description", "Connection succeeded.");
  infoObject->AddProp("clientid", _clientId);
  infoObject->AddProp("objectEncoding", objectEncoding);

  infoObject->AddProp("data", array);

  auto doc = std::make_shared<AmfDoc>();
  doc->AddProp(Rtmp::Cmd::Result);
  doc->AddProp(transacrtionId);
  doc->AddProp(propObject);
  doc->AddProp(infoObject);

  return SendAmfCmd(
      std::make_shared<RtmpMuxMsgHeader>(chunkStreamId, 0, static_cast<int>(Rtmp::MsgType::Amf0CmdMsg), _streamId, 0),
      doc);
}

//====================================================================================================
// SendAmfCreateStreamResult
//====================================================================================================
bool PlayerStream::SendAmfCreateStreamResult(uint32_t chunkStreamId, double transacrtionId) {
  auto doc = std::make_shared<AmfDoc>();

  _streamId = 1;

  doc->AddProp(Rtmp::Cmd::Result);
  doc->AddProp(transacrtionId);
  doc->AddProp(AmfDataType::Null);
  doc->AddProp(static_cast<double>(_streamId));

  return SendAmfCmd(
      std::make_shared<RtmpMuxMsgHeader>(chunkStreamId, 0, static_cast<int>(Rtmp::MsgType::Amf0CmdMsg), 0, 0), doc);
}

//====================================================================================================
// SendAmfOnStatus
//====================================================================================================
bool PlayerStream::SendAmfOnStatus(uint32_t chunkStreamId, uint32_t streamId, const char *level, const char *code,
                                   const char *description, double client_id) {

  auto object = std::make_shared<AmfObject>();
  object->AddProp("level", level);
  object->AddProp("code", code);
  object->AddProp("description", description);
  object->AddProp("clientid", client_id);

  auto doc = std::make_shared<AmfDoc>();
  doc->AddProp(Rtmp::Cmd::OnStatus);
  doc->AddProp(0.0);
  doc->AddProp(AmfDataType::Null);
  doc->AddProp(object);

  return SendAmfCmd(
      std::make_shared<RtmpMuxMsgHeader>(chunkStreamId, 0, static_cast<int>(Rtmp::MsgType::Amf0CmdMsg), streamId, 0),
      doc);
}

//====================================================================================================
// Send Meta Data
//====================================================================================================
bool PlayerStream::SendMetaData(const std::shared_ptr<AmfDoc> &doc) {
  auto header = std::make_shared<RtmpMuxMsgHeader>(static_cast<int>(Rtmp::ChunkStreamType::Stream), 0,
                                                   static_cast<int>(Rtmp::MsgType::Amf0DataMsg), _streamId, 0);

  return SendAmfCmd(std::make_shared<RtmpMuxMsgHeader>(static_cast<int>(Rtmp::ChunkStreamType::Stream), 0,
                                                       static_cast<int>(Rtmp::MsgType::Amf0DataMsg), _streamId, 0),
                    doc);
}

//====================================================================================================
// Send Video Sequence Data
//  - Control - 1 byte
//  - type - 1byte(0)
//  - 00 00 00             - 3 byte
//  - version              - 1 byte
//  - profile              - 1 byte
//  - Compatibility        - 1 byte
//  - level                - 1 byte
//  - length size in byte  - 1 byte
//  - number of sps        - 1 byte
//  - sps size             - 2 byte  *
//  - sps data             - n byte  *
//  - number of pps        - 1 byte
//  - pps size             - 2 byte  *
//  - pps data             - n byte  *
//====================================================================================================
bool PlayerStream::SendVideoSequenceData(std::shared_ptr<std::vector<uint8_t>> seqHeader) {

  return SendMsg(std::make_shared<RtmpMuxMsgHeader>(static_cast<int>(Rtmp::ChunkStreamType::Stream), 0,
                                                    static_cast<int>(Rtmp::MsgType::VideoMsg), _streamId,
                                                    seqHeader->size()),
                 seqHeader);
}

//====================================================================================================
// Send Audio Sequence Data
// -Audio Control Byte(1Byte)
//   Format(4Bit)          - 10(Aac), 2(Mp3), 11(Speex)
//   Sample rate(2Bit)    - 1(11khz), 2(22khz)/3(44Khz)
//   Sample size(1bit)    - 1(16bit) 0 (8bit)
//   Channels(1bit)        - 1(stereo) 0 (mono)
// - Aac(2Byte)
//    Format(4bit)        - 1(Aac), 2(Mp3), 11(Speex)
//    Frequency(4bit)     - 2(44100), 3(22050), 5(11025)
//    Channel(8bit)        - 16,8
//- 2Byte( AAAAABBB BCCCCDEF )
// A: 00010 - LC;
// B: 1000 - sample rate index (8 --- 16000Hz)
// C: 0001 - 1 channel (front-center);
// D: 0 - 1024 frame size;
// E: 0 - depends on core;
// F: 0 - ext. flag
//====================================================================================================
bool PlayerStream::SendAudioSequenceData(std::shared_ptr<std::vector<uint8_t>> seqHeader) {
  return SendMsg(std::make_shared<RtmpMuxMsgHeader>(static_cast<int>(Rtmp::ChunkStreamType::Stream), 0,
                                                    static_cast<int>(Rtmp::MsgType::AudioMsg), _streamId,
                                                    seqHeader->size()),
                 seqHeader);
}

//====================================================================================================
// Send Video Frame
//====================================================================================================
bool PlayerStream::SendVideo(const std::shared_ptr<Rtmp::Frame> &frame) {
  // start check
  if (!_isPlayStart) {
    return true;
  }

  // set last timestamp
  _lastVideoTimestamp = frame->timestamp;

  return SendMsg(std::make_shared<RtmpMuxMsgHeader>(static_cast<int>(Rtmp::ChunkStreamType::Stream), frame->timestamp,
                                                    static_cast<int>(Rtmp::MsgType::VideoMsg), _streamId,
                                                    frame->data->size()),
                 frame->data);
}

//====================================================================================================
// Send Audio Frame
//====================================================================================================
bool PlayerStream::SendAudio(const std::shared_ptr<Rtmp::Frame> &frame) {

  if (!_isPlayStart) {
    return true;
  }

  // set last timestamp
  _lastAudioTimestamp = frame->timestamp;

  return SendMsg(std::make_shared<RtmpMuxMsgHeader>(static_cast<int>(Rtmp::ChunkStreamType::Stream), frame->timestamp,
                                                    static_cast<int>(Rtmp::MsgType::AudioMsg), _streamId,
                                                    frame->data->size()),
                 frame->data);
}
