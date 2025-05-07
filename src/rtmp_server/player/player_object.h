#pragma once
#include "network/network_tcp_object.h"
#include "player_stream.h"
#include <memory>
#include <vector>

class PlayerEvent {
public:
  virtual ~PlayerEvent() = default;

  virtual bool OnPlayerStart(int indexKey, const std::string &streamPath) = 0;
  virtual std::shared_ptr<Rtmp::MediaInfo> OnPlayerPlay(int indexKey, const std::string &streamPath) = 0;
};

//====================================================================================================
// Rtmp Client Object
//====================================================================================================
class PlayerObject : public PlayerStreamEvent, public Network::TcpObject {
public:
  PlayerObject(const std::shared_ptr<PlayerEvent> &event) : _event(event) {}
  virtual ~PlayerObject() = default;

public:
  bool Create(const std::shared_ptr<Network::NetTcpParam> &param);

  bool SendPackt(int dataSize, uint8_t *data);
  const std::string &GetStreamPath() { return _streamPath; }

  bool SendVideo(const std::shared_ptr<Rtmp::Frame> &frame);
  bool SendAudio(const std::shared_ptr<Rtmp::Frame> &frame);

protected:
  int RecvHandler(const std::shared_ptr<std::vector<uint8_t>> &data);

  // RtmpStream implement
  bool StreamSendData(const std::shared_ptr<std::vector<uint8_t>> &data);
  bool OnStreamStart(const std::string &streamPath);
  std::shared_ptr<Rtmp::MediaInfo> OnStreamPlay(const std::string &streamPath);

private:
  std::shared_ptr<PlayerEvent> _event;
  std::shared_ptr<PlayerStream> _stream;
  std::string _streamPath;
  std::shared_ptr<Rtmp::MediaInfo> _mediaInfo = nullptr;

  uint64_t _lastAudioTimestamp = 0;
};
