#pragma once
#include "network/network_tcp_object.h"
#include "studio_stream.h"
#include <memory>
#include <vector>

struct InputStreamingInfo {
  InputStreamingInfo(const std::string &streamPath_, const std::string &remoteIp_, uint64_t sendTraffic_,
                     uint64_t recvTraffic_, uint64_t videoTimestamp_, uint64_t audioTimestamp_, double videoFps_,
                     double audioFps_)
      : streamPath(streamPath_), remoteIp(remoteIp_), sendTraffic(sendTraffic_), recvTraffic(recvTraffic_),
        videoTimestamp(videoTimestamp_), audioTimestamp(audioTimestamp_), videoFps(videoFps_), audioFps(audioFps_) {}
  std::string streamPath = nullptr;
  std::string remoteIp;
  uint64_t sendTraffic = 0;
  uint64_t recvTraffic = 0;
  uint64_t videoTimestamp = 0;
  uint64_t audioTimestamp = 0;
  double videoFps = 0;
  double audioFps = 0;
};

struct FpsCheck {
  uint32_t checkInterval = 3000; // ms
  uint64_t checkTick = 0;
  uint32_t videoFrameCount = 0;
  uint32_t audioFrameCount = 0;
  double lastVideoFps = 0;
  double lastAudioFps = 0;
};

class StudioEvent {
public:
  virtual ~StudioEvent() = default;

  virtual bool OnStudioStart(int indexKey, const std::string &streamPath) = 0;
  virtual bool OnStudioReady(int indexKey, const std::string &streamStreamPath, const std::string &streamApp,
                             const std::string &streamKey, const std::shared_ptr<Rtmp::MediaInfo> &mediaInfo) = 0;
  virtual bool OnStudioStream(int indexKey, const std::string &streamStreamPath,  const std::shared_ptr<Rtmp::Frame> &frame,
                              bool isVideo) = 0;
};

//====================================================================================================
// Rtmp Client Object
//====================================================================================================
class StudioObject : public StudioStreamEvent, public Network::TcpObject {
public:
  StudioObject(const std::shared_ptr<StudioEvent> &event) : _event(event) {}
  virtual ~StudioObject() = default;

public:
  bool Create(const std::shared_ptr<Network::NetTcpParam> &param);
  bool SendPackt(int dataSize, uint8_t *data);
  const std::string &GetStreamPath() { return _streamPath; }
  std::shared_ptr<InputStreamingInfo> GetCurrentStreamingInfo();
  bool StreamStart(const std::string &mediaId);
  bool StreamStop();

protected:
  int RecvHandler(const std::shared_ptr<std::vector<uint8_t>> &data) override;

  // RtmpEncooderStream implement
  bool StreamSendData(const std::shared_ptr<std::vector<uint8_t>> &data);
  bool OnStreamStart(const std::string &streamPath);
  bool OnStreamReady(const std::string &streamApp, const std::string &streamKey,
                     const std::shared_ptr<Rtmp::MediaInfo> &mediaInfo);
  bool OnStreamData(const std::shared_ptr<Rtmp::Frame> &frame, bool isVideo);

private:
  std::shared_ptr<StudioEvent> _event;
  std::shared_ptr<StudioStream> _stream;
  std::string _streamPath;
  std::string _mediaId;

  // Framerate check
  FpsCheck _fpsCheck;
  uint64_t _lastAudioTimestamp = 0;
};
