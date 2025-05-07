#include "studio_stream.h"
#include <string>
#include <utility>

// OnDataReceived
int StudioStream::RecvHandler(const std::shared_ptr<std::vector<uint8_t>> &data) {
  if (data->size() > Rtmp::MaxPacketSize) {
    LOG_ERROR("studio - recv data size fail - stream(%s) size(%d)", _streamPath.c_str(), data->size());
    return -1;
  }

  int procSize = (_handshakeState != Rtmp::HandshakeState::Complete) ? RecvHandshake(data) : RecvChunk(data);

  if (procSize < 0) {
    LOG_ERROR("studio - process size fail - stream(%s) size(%d)", _streamPath.c_str(), procSize);
    return -1;
  }

  return procSize;
}

// Handshake
int StudioStream::RecvHandshake(const std::shared_ptr<const std::vector<uint8_t>> &data) {
  int procSize = 0;

  if (_handshakeState == Rtmp::HandshakeState::Ready) {
    procSize = 1 + Rtmp::HandshakePacketSize; // c0 + c1
  } else if (_handshakeState == Rtmp::HandshakeState::S2) {
    procSize = Rtmp::HandshakePacketSize; // c2
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
      LOG_WRITE("studio - handshake version fail - version(%d:%d)", (*data)[0], Rtmp::HandshakeVersion);
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

// SendHandshake
bool StudioStream::SendHandshake(const std::shared_ptr<const std::vector<uint8_t>> &data) {
  _handshakeState = Rtmp::HandshakeState::C1;

  // S0 S1 S2 Send
  auto event = _event.lock();
  if (!event) {
    return false;
  }

  if (!event->StreamSendData(RtmpHandshake::MakeS0_S1_S2(data->data() + 1))) {
    LOG_ERROR("studio - handshake s0+s1+s2 send fail");
    return false;
  }

  _handshakeState = Rtmp::HandshakeState::S2;

  return true;
}

// RecvChunk
int32_t StudioStream::RecvChunk(const std::shared_ptr<const std::vector<uint8_t>> &data) {
  int procSize = 0;

  while (procSize < static_cast<int>(data->size())) {
    auto [size, isComplete] = _importChunk->ImportStreamData(data->data() + procSize, data->size() - procSize);

    if (size == 0) {
      break;
    } else if (size < 0) {
      LOG_ERROR("studio - importStream fail - stream(%s)", _streamPath.c_str());
      return size;
    }

    if (isComplete) {
      if (!ProcessChunkMsg()) {
        LOG_ERROR("Rtmp publisher - chunk message fail - stream(%s)", _streamPath.c_str());
        return -1;
      }
    }

    procSize += size;
  }

  // Acknowledgement append
  _ackTraffic += procSize;

  if (_ackTraffic > _ackSize) {
    SendAckSize();

    // Init
    _ackTraffic = 0;
  }

  return procSize;
}

// ProcessChunkMsg
bool StudioStream::ProcessChunkMsg() {
  while (true) {
    auto msg = _importChunk->GetMessage();

    if (!msg || !msg->body) {
      break;
    }

    if (msg->header->bodySize > Rtmp::MaxPacketSize) {
      LOG_ERROR("studio - packet size fail - stream(%s) size(%u:%u)", _streamPath.c_str(), msg->header->bodySize,
                Rtmp::MaxPacketSize);
      return false;
    }

    bool result = true;
    auto type = msg->header->typeId;

    if (type == static_cast<int>(Rtmp::MsgType::AudioMsg)) {
      result = OnAudioMsg(msg);
    } else if (type == static_cast<int>(Rtmp::MsgType::VideoMsg)) {
      result = OnVideoMsg(msg);
    } else if (type == static_cast<int>(Rtmp::MsgType::SetChunkSize)) {
      result = OnSetChunkSize(msg);
    } else if (type == static_cast<int>(Rtmp::MsgType::Amf0DataMsg)) {
      OnAmfDataMsg(msg);
    } else if (type == static_cast<int>(Rtmp::MsgType::Amf0CmdMsg)) {
      OnAmfCmdMsg(msg);
    } else if (type == static_cast<int>(Rtmp::MsgType::WindowAckSize)) {
      OnWindowAckSize(msg);
    } else {
      LOG_WARN("studio - unknown type - stream(%s) type(%d)", _streamPath.c_str(), msg->header->typeId);
    }

    if (!result) {
      return false;
    }
  }

  return true;
}

// OnSetChunkSize
bool StudioStream::OnSetChunkSize(const std::shared_ptr<ImportMsg> &msg) {
  auto chunkSize = RtmpMuxUtil::ReadInt32(msg->body->data());
  if (chunkSize <= 0) {
    LOG_ERROR("studio - chunkSize fail - stream(%s)", _streamPath.c_str());
    return false;
  }

  LOG_INFO("studio - set receive chunk - stream(%s) size(%u)", _streamPath.c_str(), chunkSize);
  _importChunk->SetChunkSize(chunkSize);
  return true;
}

// OnWindowAckSize
void StudioStream::OnWindowAckSize(const std::shared_ptr<ImportMsg> &msg) {
  if (auto ackledgement_size = RtmpMuxUtil::ReadInt32(msg->body->data()); ackledgement_size != 0) {
    _ackSize = ackledgement_size / 2;
    _ackTraffic = 0;
  }
}

// OnAmfCmdMsg
void StudioStream::OnAmfCmdMsg(const std::shared_ptr<ImportMsg> &msg) {
  auto doc = std::make_shared<AmfDoc>();
  if (doc->Decode(msg->body) == 0) {
    LOG_WRITE("studio - amf doc size 0 - stream(%s)", _streamPath.c_str());
    return;
  }

  std::string cmd;
  if (doc->GetProp(0) != nullptr && doc->GetProp(0)->GetType() == AmfDataType::String) {
    cmd = doc->GetProp(0)->GetString();
  }

  double transacrtionId = 0.0;
  if (doc->GetProp(1) != nullptr && doc->GetProp(1)->GetType() == AmfDataType::Number) {
    transacrtionId = doc->GetProp(1)->GetNumber();
  }

  if (cmd.empty()) {
    LOG_WRITE("studio - amf message name empty - stream(%s)", _streamPath.c_str());
    return;
  }

  if (cmd == Rtmp::Cmd::Connect) {
    OnAmfConnect(msg->header, doc, transacrtionId);
  } else if (cmd == Rtmp::Cmd::CreateStream) {
    OnAmfCreateStream(msg->header, doc, transacrtionId);
  } else if (cmd == Rtmp::Cmd::FcPublish) {
    OnAmfFCPublish(msg->header, doc, transacrtionId);
  } else if (cmd == Rtmp::Cmd::Publish) {
    OnAmfPublish(msg->header, doc, transacrtionId);
  } else if (cmd == Rtmp::Cmd::ReleaseStream) {
    // Handle release stream
  } else if (cmd == Rtmp::Cmd::Ping) {
    // Handle ping
  } else if (cmd == Rtmp::Cmd::DeleteStream) {
    OnAmfDeleteStream(msg->header, doc, transacrtionId);
  } else {
    LOG_WARN("studio - unknown amf0 cmd message - stream(%s) message(%s:%.1f)", _streamPath.c_str(), cmd.c_str(),
             transacrtionId);
  }
}

// OnAmfDataMsg
void StudioStream::OnAmfDataMsg(const std::shared_ptr<ImportMsg> &msg) {
  auto doc = std::make_shared<AmfDoc>();
  if (doc->Decode(msg->body) == 0) {
    LOG_WARN("studio - amf0 data message doc length 0 - stream(%s)", _streamPath.c_str());
    return;
  }

  std::string cmd;
  if (doc->GetProp(0) != nullptr && doc->GetProp(0)->GetType() == AmfDataType::String) {
    cmd = doc->GetProp(0)->GetString();
  }

  std::string dataName;
  if (doc->GetProp(1) != nullptr && doc->GetProp(1)->GetType() == AmfDataType::String) {
    dataName = doc->GetProp(1)->GetString();
  }

  if (cmd == Rtmp::Cmd::SetDataFrame && dataName == Rtmp::Cmd::OnMetaData) {
    OnAmfMetaData(doc);
  } else {
    LOG_WARN("studio - unknown amf0 data message - stream(%s) message(%s)", _streamPath.c_str(), cmd.c_str());
  }
}

// OnAmfConnect
void StudioStream::OnAmfConnect(const std::shared_ptr<RtmpMuxMsgHeader> &header, const std::shared_ptr<AmfDoc> &doc,
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
    LOG_ERROR("studio - send window acknowledgement size fail - stream(%s)", _streamPath.c_str());
    return;
  }

  if (!SendSetPeerBandwidth()) {
    LOG_ERROR("SendSetPeerBandwidth fail - stream(%s)", _streamPath.c_str());
    return;
  }

  if (!SendStreamBegin()) {
    LOG_ERROR("studio - send stream begin fail - stream(%s)", _streamPath.c_str());
    return;
  }

  if (!SendAmfConnectResult(header->chunkStreamId, transacrtionId, objectEncoding)) {
    LOG_ERROR("studio - send amf connect result fail - stream(%s)", _streamPath.c_str());
    return;
  }
}

// OnAmfCreateStream
void StudioStream::OnAmfCreateStream(const std::shared_ptr<RtmpMuxMsgHeader> &header,
                                     const std::shared_ptr<AmfDoc> &doc, double transacrtionId) {
  if (!SendAmfCreateStreamResult(header->chunkStreamId, transacrtionId)) {
    LOG_ERROR("studio - send amf create stream result fail - stream(%s)", _streamPath.c_str());
  }
}

// OnAmfFCPublish
void StudioStream::OnAmfFCPublish(const std::shared_ptr<RtmpMuxMsgHeader> &header, const std::shared_ptr<AmfDoc> &doc,
                                  double transacrtionId) {
  if (!SendAmfFcPublish(header->chunkStreamId, _streamId, _clientId)) {
    LOG_WRITE("studio - send amf OnFCPublish fail - stream(%s)", _streamPath.c_str());
  }
}

// OnAmfPublish
void StudioStream::OnAmfPublish(const std::shared_ptr<RtmpMuxMsgHeader> &header, const std::shared_ptr<AmfDoc> &doc,
                                double transacrtionId) {
  // app name setting
  if (_streamApp.empty() && doc->GetProp(4) != nullptr && doc->GetProp(4)->GetType() == AmfDataType::String) {
    _streamApp = doc->GetProp(4)->GetString();
  }

  // app/stream check
  if ((doc->GetProp(3) == nullptr || doc->GetProp(3)->GetType() != AmfDataType::String) || _streamApp.empty()) {
    LOG_ERROR("studio - stream name empty");

    // Reject
    SendAmfOnStatus(header->chunkStreamId, _streamId, "error", "NetStream.Publish.Rejected", "Authentication Failed.",
                    _clientId);
    return;
  }

  // stream key setting
  _streamKey = doc->GetProp(3)->GetString();
  _streamPath = _streamApp + "/" + _streamKey;
  _chunkStreamId = header->chunkStreamId;

  auto event = _event.lock();
  if (!event) {
    return;
  }

  if (!event->OnStreamStart(_streamPath)) {
    LOG_ERROR("studio - publish start fail - stream(%s)", _streamPath.c_str());

    // Reject
    SendAmfOnStatus(static_cast<uint32_t>(_chunkStreamId), _streamId, "error", "NetStream.Publish.Rejected",
                    "Authentication Failed.", _clientId);
    return;
  }

  if (!SendStreamBegin()) {
    LOG_WRITE("studio - send stream begin fail - stream(%s)", _streamPath.c_str());
    return;
  }

  if (!SendAmfOnStatus(static_cast<uint32_t>(_chunkStreamId), _streamId, "status", "NetStream.Publish.Start",
                       "Publishing", _clientId)) {
    LOG_ERROR("studio - send amfO onstatus fail - stream(%s)", _streamPath.c_str());
    return;
  }
}

// OnAmfDeleteStream
void StudioStream::OnAmfDeleteStream(const std::shared_ptr<RtmpMuxMsgHeader> &header,
                                     const std::shared_ptr<AmfDoc> &doc, double transacrtionId) {
  LOG_INFO("studio - delete stream - stream(%s)", _streamPath.c_str());
  // Handle stream deletion
}

// SendMsg
bool StudioStream::SendMsg(std::shared_ptr<RtmpMuxMsgHeader> header,
                           const std::shared_ptr<std::vector<uint8_t>> &data) {
  if (!header) {
    return false;
  }

  auto exportData = _exportChunk->ExportStreamData(header, data);

  if (!exportData || !exportData->data()) {
    return false;
  }

  auto event = _event.lock();
  if (!event) {
    return false;
  }

  return event->StreamSendData(exportData);
}

// SendAmfCmd
bool StudioStream::SendAmfCmd(std::shared_ptr<RtmpMuxMsgHeader> header, const std::shared_ptr<AmfDoc> &doc) {
  if (!header) {
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

// SendUserControlMessage
bool StudioStream::SendUserControlMsg(uint16_t msg, const std::shared_ptr<std::vector<uint8_t>> &data) {
  data->insert(data->begin(), 0);
  data->insert(data->begin(), 0);
  RtmpMuxUtil::WriteInt16(data->data(), msg);

  return SendMsg(std::make_shared<RtmpMuxMsgHeader>(static_cast<int>(Rtmp::ChunkStreamType::Urgent), 0,
                                                    static_cast<int>(Rtmp::MsgType::UserControlMsg), 0, data->size()),
                 data);
}

// SendWindowAckSize
bool StudioStream::SendWindowAckSize() {
  auto body = std::make_shared<std::vector<uint8_t>>(sizeof(int));
  RtmpMuxUtil::WriteInt32(body->data(), Rtmp::DefaultAckSize);

  return SendMsg(std::make_shared<RtmpMuxMsgHeader>(static_cast<int>(Rtmp::ChunkStreamType::Urgent), 0,
                                                    static_cast<int>(Rtmp::MsgType::WindowAckSize), _streamId,
                                                    body->size()),
                 body);
}

// SendAckSize
bool StudioStream::SendAckSize() {
  auto body = std::make_shared<std::vector<uint8_t>>(sizeof(int));
  RtmpMuxUtil::WriteInt32(body->data(), _ackTraffic);

  return SendMsg(std::make_shared<RtmpMuxMsgHeader>(static_cast<int>(Rtmp::ChunkStreamType::Urgent), 0,
                                                    static_cast<int>(Rtmp::MsgType::Ack), 0, body->size()),
                 body);
}

// SendSetPeerBandwidth
bool StudioStream::SendSetPeerBandwidth() {
  auto body = std::make_shared<std::vector<uint8_t>>(5);
  RtmpMuxUtil::WriteInt32(body->data(), _peerBandwidth);
  RtmpMuxUtil::WriteInt8(body->data() + 4, 2);

  return SendMsg(std::make_shared<RtmpMuxMsgHeader>(static_cast<int>(Rtmp::ChunkStreamType::Urgent), 0,
                                                    static_cast<int>(Rtmp::MsgType::SetPeerBandWidth), _streamId,
                                                    body->size()),
                 body);
}

// SendStreamBegin
bool StudioStream::SendStreamBegin() {
  auto body = std::make_shared<std::vector<uint8_t>>(4);

  RtmpMuxUtil::WriteInt32(body->data(), _streamId);

  return SendUserControlMsg(static_cast<int>(Rtmp::UserControlMsgType::StreamBegin), body);
}

// SendAmfConnectResult
bool StudioStream::SendAmfConnectResult(uint32_t chunkStreamId, double transacrtionId, double objectEncoding) {
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

// SendAmfFcPublish
bool StudioStream::SendAmfFcPublish(uint32_t chunkStreamId, uint32_t streamId, double client_id) {
  auto object = std::make_shared<AmfObject>();
  object->AddProp("level", "status");
  object->AddProp("code", "NetStream.Publish.Start");
  object->AddProp("description", "FCPublish");
  object->AddProp("clientid", client_id);

  auto doc = std::make_shared<AmfDoc>();
  doc->AddProp(Rtmp::Cmd::OnFcpublish);
  doc->AddProp(0.0);
  doc->AddProp(AmfDataType::Null);

  doc->AddProp(object);

  return SendAmfCmd(
      std::make_shared<RtmpMuxMsgHeader>(chunkStreamId, 0, static_cast<int>(Rtmp::MsgType::Amf0CmdMsg), _streamId, 0),
      doc);
}

// SendAmfCreateStreamResult
bool StudioStream::SendAmfCreateStreamResult(uint32_t chunkStreamId, double transacrtionId) {
  auto doc = std::make_shared<AmfDoc>();

  _streamId = 1;

  doc->AddProp(Rtmp::Cmd::Result);
  doc->AddProp(transacrtionId);
  doc->AddProp(AmfDataType::Null);
  doc->AddProp(static_cast<double>(_streamId));

  return SendAmfCmd(
      std::make_shared<RtmpMuxMsgHeader>(chunkStreamId, 0, static_cast<int>(Rtmp::MsgType::Amf0CmdMsg), 0, 0), doc);
}

// SendAmfOnStatus
bool StudioStream::SendAmfOnStatus(uint32_t chunkStreamId, uint32_t streamId, const char *level, const char *code,
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

// OnAudioMsg
bool StudioStream::OnAudioMsg(const std::shared_ptr<ImportMsg> &msg) {
  auto header = msg->header;

  // set last timestamp
  _lastAudioTimestamp = header->timestamp;

  if (header->bodySize > Rtmp::MaxPacketSize || header->bodySize < 2) {
    LOG_ERROR("studio - audio size fail - stream(%s) size(%d)", _streamPath.c_str(), header->bodySize);
    return false;
  }

  if (!_mediaInfo->audio && msg->body->at(1) == 0x00) {
    Rtmp::MediaParser parser;
    auto config = parser.AacSeqParse(msg->body);
    if (!config) {
      LOG_ERROR("studio - audio config parse fail - stream(%s)", _streamPath.c_str());
      return false;
    }

    _mediaInfo->audio = config;
    _mediaInfo->audioSeqHeader = msg->body;

    CheckStreamReady();

    return true;
  }

  if (!_mediaInfo->audio) {
    return true;
  }

  auto event = _event.lock();
  if (!event) {
    return false;
  }

  return event->OnStreamData(std::make_shared<Rtmp::Frame>(header->timestamp, msg->body), false);
}

// OnVideoMsg
bool StudioStream::OnVideoMsg(const std::shared_ptr<ImportMsg> &msg) {
  auto header = msg->header;

  // set last timestamp
  _lastVideoTimestamp = header->timestamp;

  if (header->bodySize <= Rtmp::VideoDataMinSize || header->bodySize > Rtmp::MaxPacketSize) {
    LOG_ERROR("studio - video size fail - stream(%s) size(%d)", _streamPath.c_str(), header->bodySize);
    return true;
  }

  if (!_mediaInfo->video && msg->body->at(1) == 0x00) {
    const auto codecId = (*msg->body)[0] & 0x0f;
    if (codecId != 7) {
      // TODO: 12(HEVC) 13(AV1)
      LOG_ERROR("studio - video codec fail - stream(%s)", _streamPath.c_str());
      return false;
    }

    // Codec/SPS/PPS Load
    Rtmp::MediaParser parser;
    auto config = parser.H264SeqParse(msg->body);
    if (!config) {
      LOG_ERROR("studio - video config parse fail - stream(%s)", _streamPath.c_str());
      return false;
    }

    _mediaInfo->video = config;
    _mediaInfo->videoSeqHeader = msg->body;

    CheckStreamReady();

    return true;
  }

  if (!_mediaInfo->video) {
    return true;
  }

  if (header->bodySize < Rtmp::VideoFrameIndex) {
    LOG_ERROR("studio - video body size fail - stream(%s) size(%u:%d)", _streamPath.c_str(), header->bodySize,
              Rtmp::VideoFrameIndex);
    return false;
  }

  auto event = _event.lock();
  if (!event) {
    return false;
  }

  return event->OnStreamData(std::make_shared<Rtmp::Frame>(header->timestamp, msg->body), true);
}

// CheckStreamReady
bool StudioStream::CheckStreamReady() {
  if (!_mediaInfo->video || !_mediaInfo->audio) {
    return false;
  }

  auto event = _event.lock();
  if (!event) {
    return false;
  }

  return event->OnStreamReady(_streamApp, _streamKey, _mediaInfo);
}

// OnAmfMetaData
bool StudioStream::OnAmfMetaData(const std::shared_ptr<AmfDoc> &doc) {
  if (doc->GetProp(2) == nullptr) {
    return false;
  }

  AmfObjectArray *object = nullptr;
  int index = 0;

  if (doc->GetProp(2)->GetType() == AmfDataType::Object)
    object = reinterpret_cast<AmfObjectArray *>(doc->GetProp(2)->GetObject().get());
  else
    object = reinterpret_cast<AmfObjectArray *>(doc->GetProp(2)->GetArray().get());

  auto metaInfo = std::make_shared<Rtmp::MetaInfo>();

  // DeviceType
  if ((index = object->FindName("videodevice")) >= 0 && object->GetType(index) == AmfDataType::String) {
    metaInfo->encoder = object->GetString(index); // DeviceType - XSplit
  } else if ((index = object->FindName("encoder")) >= 0 && object->GetType(index) == AmfDataType::String) {
    metaInfo->encoder = object->GetString(index); // DeviceType - OBS
  }
  metaInfo->encoder = StringHelper::Replace(metaInfo->encoder, "%", "");

  if ((index = object->FindName("framerate")) >= 0 && object->GetType(index) == AmfDataType::Number) {
    metaInfo->videoFps = object->GetNumber(index);
  } else if ((index = object->FindName("videoframerate")) >= 0 && object->GetType(index) == AmfDataType::Number) {
    metaInfo->videoFps = object->GetNumber(index);
  }

  if ((index = object->FindName("width")) >= 0 && object->GetType(index) == AmfDataType::Number) {
    metaInfo->videoWidth = object->GetNumber(index); // Width
  }

  if ((index = object->FindName("height")) >= 0 && object->GetType(index) == AmfDataType::Number) {
    metaInfo->videoHeight = object->GetNumber(index); // Height
  }

  if ((index = object->FindName("videodatarate")) >= 0 && object->GetType(index) == AmfDataType::Number) {
    metaInfo->videoBps = object->GetNumber(index); // Video Data Rate
  }

  if ((index = object->FindName("bitrate")) >= 0 && object->GetType(index) == AmfDataType::Number) {
    metaInfo->videoBps = object->GetNumber(index); // Video Data Rate
  }

  if ((index = object->FindName("maxBitrate")) >= 0 && object->GetType(index) == AmfDataType::String) {
    metaInfo->videoBps = atoi(object->GetString(index));
  }

  // audio bitrate
  if (((index = object->FindName("audiodatarate")) >= 0 || (index = object->FindName("audiobitrate")) >= 0) &&
      (object->GetType(index) == AmfDataType::Number)) {
    metaInfo->audioBps = object->GetNumber(index); // Audio Data Rate
  }

  _mediaInfo->metaInfo = metaInfo;
  _mediaInfo->metaData = doc;

  std::cout << "=== Meta Info ===" << std::endl;
  std::cout << "Encoder: " << metaInfo->encoder << std::endl;
  std::cout << "Video Fps: " << metaInfo->videoFps << std::endl;
  std::cout << "Video Bitrate: " << metaInfo->videoBps << std::endl;
  std::cout << "Video Width: " << metaInfo->videoWidth << std::endl;
  std::cout << "Video Height: " << metaInfo->videoHeight << std::endl;
  std::cout << "Audio Bitrate: " << metaInfo->audioBps << std::endl;

  return true;
}