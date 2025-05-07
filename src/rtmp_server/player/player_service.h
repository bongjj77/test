#pragma once
#include "network/network_tcp_manager.h"
#include "player_object.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

//====================================================================================================
// PlayerService
//====================================================================================================
class PlayerService : public Network::TcpManager {
public:
  PlayerService(int objectKey, const std::string &objectName, const std::shared_ptr<Network::NetEvent> &netEvent);
  int AcceptedAdd(std::shared_ptr<boost::asio::ip::tcp::socket> socket, const std::shared_ptr<PlayerEvent> &event);

  const std::string &GetStreamPath(int indexKey);
  bool SendVideo(const std::multimap<std::string, int>::iterator &begin,
                 const std::multimap<std::string, int>::iterator &end, const std::shared_ptr<Rtmp::Frame> &frame);
  bool SendAudio(const std::multimap<std::string, int>::iterator &begin,
                 const std::multimap<std::string, int>::iterator &end, const std::shared_ptr<Rtmp::Frame> &frame);
};
