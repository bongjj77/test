#include "player_service.h"

//====================================================================================================
// Constructor
//====================================================================================================
PlayerService::PlayerService(int objectKey, const std::string &objectName,
                             const std::shared_ptr<Network::NetEvent> &netEvent)
    : Network::TcpManager(objectKey, objectName, netEvent) {}

//====================================================================================================
// Object 추가(Accepted)
//====================================================================================================
int PlayerService::AcceptedAdd(std::shared_ptr<boost::asio::ip::tcp::socket> socket,
                               const std::shared_ptr<PlayerEvent> &event) {
  if (!socket) {
    return -1;
  }

  auto object = std::make_shared<PlayerObject>(event);
  if (object->Create(std::make_shared<Network::NetTcpParam>(_objectKey, _objectName, socket, _netEvent))) {
    return Insert(object, true, 5);
  }

  return -1;
}

//====================================================================================================
// Get StreamKey
//====================================================================================================
const std::string &PlayerService::GetStreamPath(int indexKey) {
  if (auto object = Find(indexKey); object != nullptr) {
    return std::static_pointer_cast<PlayerObject>(object)->GetStreamPath();
  }
  return "";
}

//====================================================================================================
// Send Video
//====================================================================================================
bool PlayerService::SendVideo(const std::multimap<std::string, int>::iterator &begin,
                              const std::multimap<std::string, int>::iterator &end,
                              const std::shared_ptr<Rtmp::Frame> &frame) {
  for (auto it = begin; it != end; ++it) {
    if (auto object = Find(it->second); object != nullptr) {
      std::static_pointer_cast<PlayerObject>(object)->SendVideo(frame);
    }
  }
  return true;
}

//====================================================================================================
// Send Audio
//====================================================================================================
bool PlayerService::SendAudio(const std::multimap<std::string, int>::iterator &begin,
                              const std::multimap<std::string, int>::iterator &end,
                              const std::shared_ptr<Rtmp::Frame> &frame) {
  for (auto it = begin; it != end; ++it) {
    if (auto object = Find(it->second); object != nullptr) {
      std::static_pointer_cast<PlayerObject>(object)->SendAudio(frame);
    }
  }
  return true;
}
