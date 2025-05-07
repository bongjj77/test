#pragma once
#include "network/network_tcp_manager.h"
#include "studio_object.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

//====================================================================================================
// Studio Service
//====================================================================================================
class StudioService : public Network::TcpManager {
public:
  StudioService(int objectKey, const std::string &objectName, const std::shared_ptr<Network::NetEvent> &netEvent);

  int AcceptedAdd(std::shared_ptr<boost::asio::ip::tcp::socket> socket, const std::shared_ptr<StudioEvent> &event);

  const std::string &GetStreamPath(int indexKey);
  bool StreamStart(const std::string &streamPath, const std::string &mediaId);
  bool StreamStop(const std::string &streamPath);

  std::shared_ptr<std::vector<std::shared_ptr<InputStreamingInfo>>> GetCurrentStreamingInfo();
};
