#pragma once
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace Network {

constexpr int DefaultSocketBufferSize = 8192;
constexpr int DefaultMaxSendQueueSize = 5000;
constexpr int DefaultMaxSendPacketSize = 25600;        // 25.6K bps
constexpr int DefaultSendTimeOut = 20;                 // seconds
constexpr int DefaultTrafficCheckTimeInterval = 30;    // milliseconds
constexpr int DefaultPostCloseTimerInterval = 1;       // milliseconds
constexpr int DefaultCloseTimerInterval = 50;          // milliseconds
constexpr int DefaultTimeoutCheckTimerInterval = 1000; // milliseconds

enum class Timer {
  KeepaliveSend,
  TimeoutCheck,
  PostClose,
};

//====================================================================================================
// Data Definition
//====================================================================================================
struct SendData {
  time_t sendTime = 0;
  std::shared_ptr<std::vector<uint8_t>> data = nullptr;

  SendData(bool isDataCopy, const std::shared_ptr<std::vector<uint8_t>> &data_)
      : data(isDataCopy ? std::make_shared<std::vector<uint8_t>>(data_->begin(), data_->end()) : data_) {}
};

enum class ConnectedResult {
  Success = 0,
  Fail,
};

struct TcpConnectedParam {
  ConnectedResult resultCode = ConnectedResult::Success;
  time_t createTime = time(nullptr);
  std::shared_ptr<boost::asio::ip::tcp::socket> socket = nullptr;
  std::string ip;
  int port;
};

class NetEvent {
public:
  virtual ~NetEvent() = default;

  virtual bool OnAccepted(int objectKey, std::shared_ptr<boost::asio::ip::tcp::socket> socket, const std::string &ip,
                          int port) = 0;
  virtual bool OnConnected(int objectKey, const std::shared_ptr<TcpConnectedParam> &connectedParam) = 0;
  virtual int OnClosed(int objectKey, int indexKey, const std::string &ip, int port) = 0;
};

} // namespace Network
