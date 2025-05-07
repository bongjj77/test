#include "studio_object.h"
#include "common/common_header.h"

//====================================================================================================
// Create
//====================================================================================================
bool StudioObject::Create(const std::shared_ptr<Network::NetTcpParam> &param) {
  if (!Network::TcpObject::Create(param)) {
    return false;
  }

  _stream = std::make_shared<StudioStream>(std::static_pointer_cast<StudioObject>(shared_from_this()));

  return true;
}

//====================================================================================================
// Stream Start
//====================================================================================================
bool StudioObject::StreamStart(const std::string &mediaId) {
  _mediaId = mediaId;

  LOG_DEBUG("*** TEST *** %s ", mediaId.c_str());
  return true;
}

//====================================================================================================
// Stream Stop
//====================================================================================================
bool StudioObject::StreamStop() {
  _mediaId = "";
  return false;
}

//====================================================================================================
// Packet Send
//====================================================================================================
bool StudioObject::SendPackt(int dataSize, uint8_t *data) {
  return PostSend(std::make_shared<std::vector<uint8_t>>(dataSize), false);
}

//====================================================================================================
//  Recv Handler
//====================================================================================================
int StudioObject::RecvHandler(const std::shared_ptr<std::vector<uint8_t>> &data) { return _stream->RecvHandler(data); }

//====================================================================================================
// stream send
//====================================================================================================
bool StudioObject::StreamSendData(const std::shared_ptr<std::vector<uint8_t>> &data) {
  if (!PostSend(data)) {
    LOG_ERROR("StreamSendData - post send fail - object(%s)", _objectName.c_str());
    return false;
  }
  return true;
}

//====================================================================================================
// stream start
//====================================================================================================
bool StudioObject::OnStreamStart(const std::string &streamPath) {
  _streamPath = streamPath;

  // Callback
  if (!_event->OnStudioStart(_indexKey, streamPath)) {
    LOG_ERROR("OnStreamStart - start fail - object(%s) stream(%s) ", _objectName.c_str(), _streamPath.c_str());
    return false;
  }
  return true;
}

//====================================================================================================
// stream ready
//====================================================================================================
bool StudioObject::OnStreamReady(const std::string &streamApp, const std::string &streamKey,
                                 const std::shared_ptr<Rtmp::MediaInfo> &mediaInfo) {
  // Callback
  if (!_event->OnStudioReady(_indexKey, _streamPath, streamApp, streamKey, mediaInfo)) {
    LOG_ERROR("OnStreamReady - ready complete fail - object(%s) stream(%s) ", _objectName.c_str(), _streamPath.c_str());
    return false;
  }
  return true;
}

//====================================================================================================
// stream data
//====================================================================================================
bool StudioObject::OnStreamData(const std::shared_ptr<Rtmp::Frame> &frame, bool isVideo) {
  if (_mediaId.empty()) {
    LOG_DEBUG("Wait mediaid -  stream(%s)", _streamPath.c_str());
    return true;
  }

  // Callback
  if (!_event->OnStudioStream(_indexKey, _streamPath, frame, isVideo)) {
    LOG_ERROR("OnStreamData - stream data fail - object(%s) stream(%s)", _objectName.c_str(), _streamPath.c_str());
    return false;
  }

  auto curTick = GetCurrentTick();

  if (_fpsCheck.checkTick == 0) {
    _fpsCheck.checkTick = curTick;
  }

  if (isVideo) {
    _fpsCheck.videoFrameCount++;
  } else {
    _fpsCheck.audioFrameCount++;
  }

  if (auto timeGap = (curTick - _fpsCheck.checkTick); timeGap >= (_fpsCheck.checkInterval)) {
    // frame rate
    _fpsCheck.lastVideoFps = _fpsCheck.videoFrameCount / (static_cast<double>(timeGap) / 1000);
    _fpsCheck.lastAudioFps = _fpsCheck.audioFrameCount / (static_cast<double>(timeGap) / 1000);

    _fpsCheck.audioFrameCount = 0;
    _fpsCheck.videoFrameCount = 0;
    _fpsCheck.checkTick = curTick;
  }

  return true;
}

//====================================================================================================
// GetCurrentStreamingInfo
//====================================================================================================
std::shared_ptr<InputStreamingInfo> StudioObject::GetCurrentStreamingInfo() {
  return std::make_shared<InputStreamingInfo>(_streamPath, _ip, GetSendTraffic(), GetRecvTraffic(),
                                              _stream->GetLastVideoTimestamp(), _stream->GetLastAudioTimestamp(),
                                              _fpsCheck.lastVideoFps, _fpsCheck.lastAudioFps);
}
