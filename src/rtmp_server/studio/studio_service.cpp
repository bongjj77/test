#include "studio_service.h"

//====================================================================================================
// Constructor
//====================================================================================================
StudioService::StudioService(int objectKey, const std::string &objectName,
                             const std::shared_ptr<Network::NetEvent> &netEvent)
    : Network::TcpManager(objectKey, objectName, netEvent) {}

//====================================================================================================
// Object 추가(Accepted)
//====================================================================================================
int StudioService::AcceptedAdd(std::shared_ptr<boost::asio::ip::tcp::socket> socket,
                               const std::shared_ptr<StudioEvent> &event) {
  if (!socket) {
    return -1;
  }

  auto object = std::make_shared<StudioObject>(event);
  if (object->Create(std::make_shared<Network::NetTcpParam>(_objectKey, _objectName, socket, _netEvent))) {
    return Insert(object, true, 5);
  }
  return -1;
}

//====================================================================================================
// Get StreamPath
//====================================================================================================
const std::string &StudioService::GetStreamPath(int indexKey) {
  if (auto object = Find(indexKey); object != nullptr) {
    return std::static_pointer_cast<StudioObject>(object)->GetStreamPath();
  }
  return "";
}
//====================================================================================================
// Stream start
//====================================================================================================
bool StudioService::StreamStart(const std::string &streamPath, const std::string &mediaId) {
  std::lock_guard<std::mutex> lock(_netObjectsLock);
  for (const auto &[indexKey, object] : _netObjects) {
    if (std::static_pointer_cast<StudioObject>(object)->GetStreamPath() == streamPath) {
      return std::static_pointer_cast<StudioObject>(object)->StreamStart(mediaId);
    }
  }
  return false;
}

//====================================================================================================
// Stream stop
//====================================================================================================
bool StudioService::StreamStop(const std::string &streamPath) {
  std::lock_guard<std::mutex> lock(_netObjectsLock);
  for (const auto &[indexKey, object] : _netObjects) {
    if (std::static_pointer_cast<StudioObject>(object)->GetStreamPath() == streamPath) {
      return std::static_pointer_cast<StudioObject>(object)->StreamStop();
    }
  }
  return false;
}

//====================================================================================================
// Get Current Streaming Info
//====================================================================================================
std::shared_ptr<std::vector<std::shared_ptr<InputStreamingInfo>>> StudioService::GetCurrentStreamingInfo() {
  auto infos = std::make_shared<std::vector<std::shared_ptr<InputStreamingInfo>>>();

  std::lock_guard<std::mutex> networkLock(_netObjectsLock);

  for (const auto &[indexKey, object] : _netObjects) {
    infos->push_back(std::static_pointer_cast<StudioObject>(object)->GetCurrentStreamingInfo());
  }

  return infos;
}
