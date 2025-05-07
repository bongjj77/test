#pragma once
#include "common/system_monitor.h"
#include "common/timer_manager.h"
#include "controller/controller.h"
#include "media/rtmp/rtmp_media_parser.h"
#include "network/network_header.h"
#include "network/network_manager.h"
#include "player/player_service.h"
#include "studio/studio_service.h"
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
extern "C" {
#include "../external/redis/include/alloc.h"
#include "../external/redis/include/async.h"
#include "../external/redis/include/hiredis.h"
#include "../external/redis/include/read.h"
#include "../external/redis/include/sds.h"
}
#else
#include <hiredis/alloc.h>
#include <hiredis/async.h>
#include <hiredis/hiredis.h>
#include <hiredis/read.h>
#include <hiredis/sds.h>
#endif

enum class NetObjectKey { Studio, Player };

//===============================================================================================
// Config
//===============================================================================================
struct Config {
  std::string version;
  std::string hostIp;
  int studioPort;
  int playerPort;
  std::string controllerHost;
  int controllerPort;

  std::string ToString() const {
    std::ostringstream oss;
    oss << "  - Version : " << version << std::endl;
    oss << "  - Host ip : " << hostIp << std::endl;
    oss << "  - Studio port : " << playerPort << std::endl;
    oss << "  - Player port : " << playerPort << std::endl;
    oss << "  - Controller : " << controllerHost << ":" << controllerPort << std::endl;
    return oss.str();
  }
};

//===============================================================================================
// MainObject
//===============================================================================================
class MainObject : public StudioEvent,
                   public PlayerEvent,
                   public ControllerEvent,
                   public Network::NetEvent,
                   public std::enable_shared_from_this<MainObject> {
public:
  MainObject();
  virtual ~MainObject();

  bool Create(std::unique_ptr<Config> param);
  void Release();
  bool ConnectToRedis(const std::string &host, int port);               // Redis 연결 메서드
  bool SetRedisValue(const std::string &key, const std::string &value); // Redis SET 명령 메서드

private:
  std::string GetNetObjectName(NetObjectKey objectKey);
  std::shared_ptr<Network::TcpManager> GetNetworkManager(NetObjectKey objectKey) const;

  // Network implement
  bool OnAccepted(int objectKey, std::shared_ptr<boost::asio::ip::tcp::socket> socket, const std::string &ip, int port);
  bool OnConnected(int objectKey, const std::shared_ptr<Network::TcpConnectedParam> &connectedParam);
  int OnClosed(int objectKey, int indexKey, const std::string &ip, int port);

  // Studio implement
  bool OnStudioStart(int indexKey, const std::string &streamPath);
  bool OnStudioReady(int indexKey, const std::string &streamPath, const std::string &streamApp,
                     const std::string &streamKey, const std::shared_ptr<Rtmp::MediaInfo> &mediaInfo);
  bool OnStudioStream(int indexKey, const std::string &streamPath, const std::shared_ptr<Rtmp::Frame> &frame,
                      bool isVideo);

  // Player implement
  bool OnPlayerStart(int indexKey, const std::string &streamPath);
  std::shared_ptr<Rtmp::MediaInfo> OnPlayerPlay(int indexKey, const std::string &streamPath);

  // Controller implement
  void OnControllerStreamStart(const std::string &streamPath, const std::string &mediaId);
  void OnControllerStreamStop(const std::string &streamPath);

  void OnGarbageCheckTimer();
  void OnInfoPrintTimer();

private:
  std::unique_ptr<Config> _config;
  std::shared_ptr<StudioService> _studios;
  std::shared_ptr<PlayerService> _players;
  std::shared_ptr<Network::ContextPool> _netPool;
  std::shared_ptr<Controller> _controller;
  std::thread _controllerThread;
  TimerManager _timer;

  std::map<std::string, std::shared_ptr<Rtmp::MediaInfo>> _mediaInfos;
  mutable std::mutex _mediaInfosLock;

  std::multimap<std::string, int> _playerStreams;
  mutable std::mutex _playerStreamsLock;

  redisContext *_redisContext; // Redis 컨텍스트
};
