#include "player_object.h"

//====================================================================================================
// Create
//====================================================================================================
bool PlayerObject::Create(const std::shared_ptr<Network::NetTcpParam> &param) {
  if (!Network::TcpObject::Create(param)) {
    return false;
  }

  _stream = std::make_shared<PlayerStream>(std::static_pointer_cast<PlayerObject>(shared_from_this()));
  return true;
}
//====================================================================================================
// Packet Send
//====================================================================================================
bool PlayerObject::SendPackt(int dataSize, uint8_t *data) {
  return PostSend(std::make_shared<std::vector<uint8_t>>(dataSize), false);
}

//====================================================================================================
//  Recv Handler
//====================================================================================================
int PlayerObject::RecvHandler(const std::shared_ptr<std::vector<uint8_t>> &data) { return _stream->RecvHandler(data); }

//====================================================================================================
// stream send
//====================================================================================================
bool PlayerObject::StreamSendData(const std::shared_ptr<std::vector<uint8_t>> &data) {
  if (!PostSend(data)) {
    LOG_ERROR("StreamSendData - post send fail - object(%s) ip(%s)", _objectName.c_str(), _ip.c_str());
    return false;
  }

  return true;
}

//====================================================================================================
// stream start
//====================================================================================================
bool PlayerObject::OnStreamStart(const std::string &streamPath) {
  _streamPath = streamPath;

  // Callback
  if (!_event->OnPlayerStart(_indexKey, _streamPath)) {
    LOG_ERROR("OnStreamStart - start fail - object(%s) stream(%s) ip(%s) type(%s)", _objectName.c_str(),
              streamPath.c_str(), _ip.c_str());
    return false;
  }

  return true;
}

//====================================================================================================
// play event
//====================================================================================================
std::shared_ptr<Rtmp::MediaInfo> PlayerObject::OnStreamPlay(const std::string &streamPath) {
  _streamPath = streamPath;
  return _event->OnPlayerPlay(_indexKey, _streamPath);
}

//====================================================================================================
// Send video
//====================================================================================================
bool PlayerObject::SendVideo(const std::shared_ptr<Rtmp::Frame> &frame) { return _stream->SendVideo(frame); }

//====================================================================================================
// Send audio
//====================================================================================================
bool PlayerObject::SendAudio(const std::shared_ptr<Rtmp::Frame> &frame) { return _stream->SendAudio(frame); }
