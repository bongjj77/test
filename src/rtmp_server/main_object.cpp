#include "main_object.h"
#include <condition_variable>
#include <iomanip>
#include <utility>

MainObject::MainObject() {
  _redisContext = nullptr;
  _playerStreams.clear();
  _mediaInfos.clear();
}

MainObject::~MainObject() {
  Release();
  if (_controllerThread.joinable()) {
    _controllerThread.join();
  }

  _playerStreams.clear();
  _mediaInfos.clear();
}

void MainObject::Release() {
  LOG_INFO("Release main object");

  if (_controller) {
    if (_controllerThread.joinable()) {
      _controllerThread.join();
    }
    _controller.reset();
    LOG_INFO("Controller released");
  }

  if (_studios != nullptr) {
    _studios->PostRelease();
    LOG_INFO("StudioService released");
  }

  if (_players != nullptr) {
    _players->PostRelease();
    LOG_INFO("PlayerService released");
  }

  LOG_INFO("Network object close completed");

  if (_netPool != nullptr) {
    _netPool->Stop();
    LOG_INFO("Network service pool close completed");
  }

  if (_redisContext) {
    redisFree(_redisContext); // Redis 연결 해제
    _redisContext = nullptr;
  }
}

bool MainObject::ConnectToRedis(const std::string &host, int port) {
  _redisContext = redisConnect(host.c_str(), port);
  if (_redisContext == nullptr || _redisContext->err) {
    if (_redisContext) {
      LOG_ERROR("Redis connection error: %s", _redisContext->errstr);
      redisFree(_redisContext);
      _redisContext = nullptr;
    } else {
      LOG_ERROR("Redis connection error: can't allocate redis context");
    }
    return false;
  }
  LOG_INFO("Connected to Redis server at %s:%d", host.c_str(), port);
  return true;
}

bool MainObject::SetRedisValue(const std::string &key, const std::string &value) {
  if (_redisContext == nullptr) {
    LOG_ERROR("Redis context is not initialized");
    return false;
  }

  redisReply *reply = (redisReply *)redisCommand(_redisContext, "SET %s %s", key.c_str(), value.c_str());
  if (reply == nullptr) {
    LOG_ERROR("Redis command error: %s", _redisContext->errstr);
    return false;
  }

  LOG_INFO("Set key %s with value %s", key.c_str(), value.c_str());
  freeReplyObject(reply);
  return true;
}

//===============================================================================================
// GetNetObjectName
//===============================================================================================
std::string MainObject::GetNetObjectName(NetObjectKey objectKey) {
  switch (objectKey) {
  case NetObjectKey::Studio:
    return "Studio";
  case NetObjectKey::Player:
    return "Player";
  default:
    return "Unknown";
  }
}

//====================================================================================================
// Get network manager
//====================================================================================================
std::shared_ptr<Network::TcpManager> MainObject::GetNetworkManager(NetObjectKey objectKey) const {
  switch (objectKey) {
  case NetObjectKey::Studio:
    return _studios;
  case NetObjectKey::Player:
    return _players;
  default:
    return nullptr;
  }
}

bool MainObject::Create(std::unique_ptr<Config> param) {
  _config = std::move(param);

  // // Redis 연결
  // if (!ConnectToRedis("127.0.0.1", 6379)) {
  //   LOG_ERROR("Failed to connect to Redis server");
  //   return false;
  // }

  // // Redis에 값 설정
  // if (!SetRedisValue("test_key", "test_value")) {
  //   LOG_ERROR("Failed to set value in Redis");
  //   return false;
  // }

  // IoService start
  _netPool = std::make_shared<Network::ContextPool>(0);
  _netPool->Run();

  const auto &self = shared_from_this();

  // Studio 생성
  _studios = std::make_shared<StudioService>(static_cast<int>(NetObjectKey::Studio),
                                             GetNetObjectName(NetObjectKey::Studio), self);
  if (!_studios->Create(_netPool, _config->studioPort)) {
    LOG_ERROR("Create fail - object(%s)", _studios->GetObjectName().c_str());
    return false;
  }

  // Player 생성
  _players = std::make_shared<PlayerService>(static_cast<int>(NetObjectKey::Player),
                                             GetNetObjectName(NetObjectKey::Player), self);

  if (!_players->Create(_netPool, _config->playerPort)) {
    LOG_ERROR("Create fail - object(%s)", _players->GetObjectName().c_str());
    return false;
  }

  // Controller 생성
  _controller = std::make_shared<Controller>(_config->controllerHost, _config->controllerPort, _netPool->GetContext(),
                                             _config->hostIp, _config->playerPort, self);
  _controllerThread = std::thread([this]() { _controller->Connect(); });

  // Timer 설정
  _timer.AddTimer(20000, [this]() { OnGarbageCheckTimer(); });
  _timer.AddTimer(30000, [this]() { OnInfoPrintTimer(); });

  return true;
}

//====================================================================================================
// Network accepted callback
//====================================================================================================
bool MainObject::OnAccepted(int objectKey, std::shared_ptr<boost::asio::ip::tcp::socket> socket, const std::string &ip,
                            int port) {
  LOG_INFO("Network accepted - objectKey(%d) address(%s:%d)", objectKey, ip.c_str(), port);
  int indexKey = -1;

  try {
    switch (objectKey) {
    case static_cast<int>(NetObjectKey::Studio): {
      indexKey = _studios->AcceptedAdd(socket, shared_from_this());
      break;
    }
    case static_cast<int>(NetObjectKey::Player): {
      indexKey = _players->AcceptedAdd(socket, shared_from_this());
      break;
    }
    default: {
      LOG_WARN("Network accepted - unknown object - objectKey(%d) address(%s:%d)",
               GetNetObjectName(static_cast<NetObjectKey>(objectKey)).c_str(), ip.c_str(), port);
      break;
    }
    }
  } catch (const std::exception &e) {
    LOG_ERROR("Exception in OnNetAccepted: %s", e.what());
    return false;
  }

  if (indexKey == -1) {
    LOG_ERROR("Network accepted - object add fail - object(%s) index(%d) address(%s:%d)",
              GetNetObjectName(static_cast<NetObjectKey>(objectKey)).c_str(), indexKey, ip.c_str(), port);
    return false;
  }

  return true;
}

//====================================================================================================
// Network connected callback
//====================================================================================================
bool MainObject::OnConnected(int objectKey, const std::shared_ptr<Network::TcpConnectedParam> &connectedParam) {
  LOG_INFO("Network connected - object(%s) address(%s:%d)",
           GetNetObjectName(static_cast<NetObjectKey>(objectKey)).c_str(), connectedParam->ip.c_str(),
           connectedParam->port);

  return true;
}

//====================================================================================================
// Network closed callback
//====================================================================================================
int MainObject::OnClosed(int objectKey, int indexKey, const std::string &ip, int port) {
  LOG_INFO("Network close - object(%s) index(%d) address(%s:%d)",
           GetNetObjectName(static_cast<NetObjectKey>(objectKey)).c_str(), indexKey, ip.c_str(), port);

  std::string streamPath = "";

  if (objectKey == static_cast<int>(NetObjectKey::Studio)) {
    streamPath = _studios->GetStreamPath(indexKey);
  }

  // remove session
  LOG_INFO("Removing session - object(%s) index(%d)", GetNetObjectName(static_cast<NetObjectKey>(objectKey)).c_str(),
           indexKey);
  GetNetworkManager(static_cast<NetObjectKey>(objectKey))->Remove(indexKey);

  // stream manager release
  if (!streamPath.empty()) {
    // --- Lock Block ---
    {
      std::lock_guard<std::mutex> lock(_mediaInfosLock);
      _mediaInfos.erase(streamPath);
    }

    // --- Lock Block ---
    {
      std::lock_guard<std::mutex> lock(_playerStreamsLock);
      _playerStreams.erase(streamPath);
    }

    // TODO: 연결 Player 접속 제거
  }

  return 0;
}

//====================================================================================================
// Studio implement
//====================================================================================================
bool MainObject::OnStudioStart(int indexKey, const std::string &streamPath) {
  LOG_INFO("Step 1. studio start - index(%d) stream(%s )", indexKey, streamPath.c_str());
  return true;
}

//====================================================================================================
// Studio implement
//====================================================================================================
bool MainObject::OnStudioReady(int indexKey, const std::string &streamPath, const std::string &streamApp,
                               const std::string &streamKey, const std::shared_ptr<Rtmp::MediaInfo> &mediaInfo) {
  LOG_INFO("Step 2. studio ready complete - index(%d) stream(%s) %s", indexKey, streamPath.c_str(),
           mediaInfo->ToString().c_str());

  // set stream list
  // --- Lock Block ---
  {
    std::lock_guard<std::mutex> lock(_mediaInfosLock);
    _mediaInfos[streamPath] = mediaInfo;
  }

  // Controller에 스트림 시적 전송
  _controller->SendStreamStart(streamPath, streamApp, streamKey);

  return true;
}

//====================================================================================================
// Studio implement
//====================================================================================================
bool MainObject::OnStudioStream(int indexKey, const std::string &streamPath, const std::shared_ptr<Rtmp::Frame> &frame,
                                bool isVideo) {
  std::pair<std::multimap<std::string, int>::iterator, std::multimap<std::string, int>::iterator> players;

  //--- Lock Block ---
  {
    std::lock_guard<std::mutex> lock(_playerStreamsLock);
    players = _playerStreams.equal_range(streamPath);
  }

  // Player에 데이터 전달
  if (players.first != players.second) {
    if (isVideo) {
      _players->SendVideo(players.first, players.second, frame);
    } else {
      _players->SendAudio(players.first, players.second, frame);
    }
  }

  return true;
}

//====================================================================================================
// Player implement
//====================================================================================================
bool MainObject::OnPlayerStart(int indexKey, const std::string &streamPath) {
  LOG_INFO("Step 1. player start - index(%d) stream(%s)", indexKey, streamPath.c_str());

  return true;
}

//====================================================================================================
// Player implement
//====================================================================================================
std::shared_ptr<Rtmp::MediaInfo> MainObject::OnPlayerPlay(int indexKey, const std::string &streamPath) {
  LOG_WRITE("Step 2. player play - index(%d)  stream(%s)", indexKey, streamPath.c_str());

  std::shared_ptr<Rtmp::MediaInfo> mediaInfo;

  // --- Block Lock ---
  {
    std::lock_guard<std::mutex> lock(_mediaInfosLock);
    mediaInfo = _mediaInfos[streamPath];
  }

  // --- Block Lock ---
  {
    if (mediaInfo != nullptr) {
      std::lock_guard<std::mutex> lock(_playerStreamsLock);
      _playerStreams.insert(std::make_pair(streamPath, indexKey));
    }
  }
  return mediaInfo;
}

//====================================================================================================
// Garbage Check Proc
//====================================================================================================
void MainObject::OnGarbageCheckTimer() {
  // Implement garbage collection logic if needed
}

//====================================================================================================
// Information Print Proc
//====================================================================================================
void MainObject::OnInfoPrintTimer() {
  LOG_INFO("Connected - encoder(%d) player(%d) controller(%s)", _studios->GetCount(), _players->GetCount(),
           _controller->IsConnected() ? "true" : "false");
}

//====================================================================================================
// Stram start event from controller
//====================================================================================================
void MainObject::OnControllerStreamStart(const std::string &streamPath, const std::string &mediaId) {
  LOG_INFO("Stream start - stream(%s) media(%s)", streamPath.c_str(), mediaId.c_str());
  _studios->StreamStart(streamPath, mediaId);
}

//====================================================================================================
// Stram stop event from controller
//====================================================================================================
void MainObject::OnControllerStreamStop(const std::string &streamPath) {
  LOG_INFO("Stream stop - stream(%s)", streamPath.c_str());
  _studios->StreamStop(streamPath);
}
