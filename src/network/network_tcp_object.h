#pragma once
#include "network_context_pool.h"
#include "network_header.h"
#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Network {

class TcpObject; // Forward declaration

using TcpKeepaliveSendCallback = bool (TcpObject::*)();

struct NetTcpParam {
  NetTcpParam(int objectKey_, const std::string &objectName_,
              const std::shared_ptr<boost::asio::ip::tcp::socket> &socket_, const std::weak_ptr<NetEvent> &netEvent_)
      : objectKey(objectKey_), objectName(objectName_), socket(socket_), netEvent(netEvent_) {}

  int objectKey;
  std::string objectName;
  std::shared_ptr<boost::asio::ip::tcp::socket> socket;
  std::weak_ptr<NetEvent> netEvent;
};

class TcpObject : public std::enable_shared_from_this<TcpObject> {
public:
  TcpObject();
  virtual ~TcpObject();

  virtual bool Create(const std::shared_ptr<NetTcpParam> &param);
  virtual bool Start();
  virtual void SetRecvTimeout(uint32_t maxWaitTime);
  void SetRecvBufferSize(int bufferSize) { _recvBuffer->resize(bufferSize, 0); }
  void SetIndexKey(int indexKey) { _indexKey = indexKey; }
  int GetIndexKey() const { return _indexKey; }
  std::string GetRemoteIp() const { return _ip; }
  int GetRemotePort() const { return _port; }
  virtual time_t GetCreateTime() const { return _createTime; }
  void SetLogLock(bool lock) { _logLock = lock; }
  virtual bool PostSend(std::shared_ptr<std::vector<uint8_t>> data, bool isDataCopy = false);
  virtual void PostClose();
  virtual void Close();
  time_t GetSendCompleteTime() const { return _sendCompleteTime; }
  std::pair<uint64_t, uint64_t> GetTrafficRate(bool init);
  uint64_t GetRecvTraffic() const { return _recvTraffic; }
  uint64_t GetSendTraffic() const { return _sendTraffic; }
  uint64_t GetSendBufferSize() const { return _sendBufferSize; }
  uint64_t GetQosSendWaitTime(uint32_t checkBufferSize, uint32_t maxWaitTime, uint32_t megaPerWait);
  virtual bool IsOpened() const;

protected:
  virtual void OnReceive(const boost::system::error_code &error, size_t dataSize);
  void OnSend(const boost::system::error_code &error, size_t dataSize);
  void SendQueueRelease();
  void SetNetworkTimer(std::shared_ptr<boost::asio::steady_timer> networkTimer, Timer id, int interval);
  void OnNetworkTimer(const boost::system::error_code &error, std::shared_ptr<boost::asio::steady_timer> networkTimer,
                      Timer id, int interval);
  bool PostCloseTimerProc();

  virtual int RecvHandler(const std::shared_ptr<std::vector<uint8_t>> &data) = 0;

  virtual boost::asio::io_context &GetIoContext();
  virtual void AsyncRecv();
  virtual void AsyncSend(const std::shared_ptr<SendData> &sendData);

protected:
  int _objectKey = -1;
  int _indexKey = -1;
  std::shared_ptr<boost::asio::ip::tcp::socket> _socket = nullptr;
  std::weak_ptr<NetEvent> _netEvent;
  std::string _ip = "";
  int _port = 0;
  std::string _objectName = "unknown";

  std::deque<std::shared_ptr<SendData>> _sendQueue;
  uint64_t _sendBufferSize = 0;
  std::mutex _sendQueueLock;

  std::shared_ptr<std::vector<uint8_t>> _recvBuffer = std::make_shared<std::vector<uint8_t>>(8192, 0);
  std::shared_ptr<std::vector<uint8_t>> _recvData = nullptr;
  std::shared_ptr<std::vector<uint8_t>> _restData = nullptr;

  bool _isClosing = false;
  TcpKeepaliveSendCallback _keepaliveSendCallback = nullptr;
  uint32_t _timeout = 0;
  time_t _lastRecvTime = time(nullptr);

  std::shared_ptr<boost::asio::steady_timer> _keepaliveSendTimer = nullptr;
  std::shared_ptr<boost::asio::steady_timer> _timeoutTimer = nullptr;
  std::shared_ptr<boost::asio::steady_timer> _postCloseTimer = nullptr;

  time_t _createTime = time(nullptr);
  time_t _sendCompleteTime = 0;
  bool _logLock = false;
  bool _networkError = false;
  bool _isSendCompletedClose = false;

  int64_t _trafficCheckTime = GetCurrentMs(); // ms
  uint64_t _sendTraffic = 0;                  // byte
  uint64_t _recvTraffic = 0;                  // byte
  int _postCloseTimeInterval = DefaultPostCloseTimerInterval;
};

} // namespace Network
