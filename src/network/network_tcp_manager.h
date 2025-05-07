#pragma once
#include "network_manager.h"
#include "network_tcp_object.h"
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Network {

class TcpManager : public Manager {
public:
  TcpManager(int objectKey, const std::string &objectName, const std::shared_ptr<NetEvent> &event);
  virtual ~TcpManager();

public:
  virtual bool Create(std::shared_ptr<ContextPool> servicePool, int listenPort = 0, bool privateAccepter = false);
  virtual bool PostConnect(const std::string &ip, int port, std::shared_ptr<TcpConnectedParam> connectedParam);

  void Close(int indexKey) { Remove(indexKey); }
  int FindIndexKey(const std::string &ip, int port);
  bool IsConnected(const std::string &ip, int port);

  void PostRelease() override;
  bool Remove(int indexKey) override;
  void Remove(const std::vector<int> &indexKeys) override;
  void RemoveAll() override;
  uint32_t GetCount() const override;

  bool IsServerModule() const { return (_listenPort != 0); }
  bool IsClientModule() const { return (_listenPort == 0); }
  int GetCurrentIndexKey() const { return _indexKey; }

  std::pair<uint64_t, uint64_t> GetTotalTrafficRate(bool init);
  std::pair<uint64_t, uint64_t> GetTrafficRate(int indexKey, bool init);
  uint64_t GetSendTraffic(int indexKey);
  uint64_t GetSendBufferSize(int indexKey);
  uint64_t GetQosSendWaitTime(int indexKey, uint32_t check_buffer_size = 10 * 1024 * 1024, uint32_t max_wait_time = 300,
                              uint32_t mega_per_wait = 5);

  static std::string GetLocalIP();

protected:
  virtual void Release();
  virtual int Insert(std::shared_ptr<TcpObject> object, bool recvTimeout = false, uint32_t timeout = 0);
  std::shared_ptr<TcpObject> Find(int indexKey, bool erase = false);
  virtual bool PostAccept();
  virtual void OnAccept(const boost::system::error_code &error);
  virtual void OnConnected(const boost::system::error_code &error, std::shared_ptr<boost::asio::ip::tcp::socket> socket,
                           std::shared_ptr<TcpConnectedParam> connectedParam, const std::string &ip, int port);

protected:
  bool _isPrivateAccepter = false;

  int _indexKey = 0;
  uint32_t _maxCreatingCount = 0;
  std::map<int, std::shared_ptr<TcpObject>> _netObjects;
  mutable std::mutex _netObjectsLock;

  int _listenPort = 0;
  std::shared_ptr<boost::asio::ip::tcp::acceptor> _acceptor = nullptr;
  std::shared_ptr<boost::asio::ip::tcp::socket> _acceptSocket = nullptr;
  bool _isClosing = false;
};

} // namespace Network
