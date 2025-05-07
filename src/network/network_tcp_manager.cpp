#include "network_tcp_manager.h"
#include "common/common_header.h"

namespace Network {

TcpManager::TcpManager(int objectKey, const std::string &objectName, const std::shared_ptr<NetEvent> &event)
    : Manager(objectKey, objectName, event) {}

TcpManager::~TcpManager() { _acceptSocket = nullptr; }

void TcpManager::PostRelease() {
  _isClosing = true;
  Release();
}

void TcpManager::Release() {
  RemoveAll();

  if (_acceptor != nullptr) {
    _acceptor->close();
    _acceptor = nullptr;
  }

  if (_acceptSocket != nullptr) {
    _acceptSocket->close();
  }
}

bool TcpManager::Create(std::shared_ptr<ContextPool> servicePool, int listenPort, bool privateAccepter) {
  _servicePool = servicePool;

  if (listenPort != 0) {
    _listenPort = listenPort;
    _isPrivateAccepter = privateAccepter;
    if (!PostAccept()) {
      return false;
    }
  }

  return true;
}

int TcpManager::Insert(std::shared_ptr<TcpObject> object, bool recvTimeout, uint32_t timeout) {
  int indexKey = -1;

  if (object == nullptr) {
    LOG_ERROR("Insert parameter null - manager(%s)", _objectName.c_str());
    return -1;
  }

  std::lock_guard<std::mutex> lock(_netObjectsLock);

  if (_indexKey >= INT_MAX) {
    _indexKey = 0;
  }

  if (_netObjects.count(_indexKey) != 0) {
    bool findIndex = false;

    for (int index = _indexKey; index < INT_MAX; index++) {
      if (_netObjects.count(index) == 0) {
        _indexKey = index;
        findIndex = true;
        break;
      }
    }

    if (!findIndex) {
      LOG_ERROR("Insert - indexKey over - manager(%s)", _objectName.c_str());
    }
  }

  if (_netObjects.count(_indexKey) != 0) {
    LOG_ERROR("Insert - exist indexKey - manager(%s)", _objectName.c_str());
    return -1;
  }

  _netObjects.emplace(_indexKey, object);
  indexKey = _indexKey;
  object->SetIndexKey(_indexKey);

  if (_netObjects.size() > _maxCreatingCount) {
    _maxCreatingCount = static_cast<uint32_t>(_netObjects.size());
  }

  if (!object->Start()) {
    LOG_ERROR("Insert - object start fail - manager(%s)", _objectName.c_str());
    indexKey = -1;
  }

  if (recvTimeout && timeout != 0 && indexKey != -1) {
    object->SetRecvTimeout(timeout);
  }

  if (!object->IsOpened()) {
    LOG_ERROR("Insert - object socket close - manager(%s)", _objectName.c_str());
    indexKey = -1;
  }

  if (indexKey == -1) {
    object->PostClose();
    _netObjects.erase(_indexKey);
  }

  _indexKey++;
  return indexKey;
}

bool TcpManager::PostConnect(const std::string &ip, int port, std::shared_ptr<TcpConnectedParam> connectedParam) {
  auto socket = std::make_shared<boost::asio::ip::tcp::socket>(*(_servicePool->GetContext()));
  auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip), port);
  socket->async_connect(endpoint, [this, socket, connectedParam, ip, port](const boost::system::error_code &error) {
    OnConnected(error, socket, connectedParam, ip, port);
  });

  return true;
}

void TcpManager::OnConnected(const boost::system::error_code &error,
                             std::shared_ptr<boost::asio::ip::tcp::socket> socket,
                             std::shared_ptr<TcpConnectedParam> connectedParam, const std::string &ip, int port) {
  ConnectedResult resultCode = ConnectedResult::Success;

  if (error) {
    LOG_ERROR("OnConnected - manager(%s) address(%s:%d) error(%d) message(%s) ", _objectName.c_str(), ip.c_str(), port,
              error.value(), error.message().c_str());

    if (socket != nullptr) {
      socket->close();
      socket = nullptr;
    }
    resultCode = ConnectedResult::Fail;
  }

  connectedParam->resultCode = resultCode;
  connectedParam->socket = socket;
  connectedParam->ip = ip;
  connectedParam->port = port;

  if (auto netEvent = _netEvent.lock()) {
    if (!netEvent->OnConnected(_objectKey, connectedParam)) {
      LOG_ERROR("OnConnected - connected callback fail - manager(%s) address(%s:%d)", _objectName.c_str(), ip.c_str(),
                port);
    }
  }
}

bool TcpManager::PostAccept() {
  if (_acceptor == nullptr) {
    try {
      boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), _listenPort);

      _acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(
          _isPrivateAccepter ? *(_servicePool->GetPrivateContext()) : *(_servicePool->GetContext()));

      _acceptor->open(endpoint.protocol());
      _acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(false));
      _acceptor->bind(endpoint);
      _acceptor->listen();
    } catch (const std::exception &e) {
      LOG_ERROR("PostAccept - accept ready(Listen) fail - manager(%s) error(%s)", _objectName.c_str(), e.what());
      return false;
    }
  }

  if (_acceptSocket != nullptr) {
    LOG_WARN("PostAccept - _acceptSocket not null - manager(%s)", _objectName.c_str());
    _acceptSocket->close();
    _acceptSocket = nullptr;
  }

  _acceptSocket = std::make_shared<boost::asio::ip::tcp::socket>(*_servicePool->GetContext());

  _acceptor->async_accept(*_acceptSocket, [this](const boost::system::error_code &error) { OnAccept(error); });

  return true;
}

void TcpManager::OnAccept(const boost::system::error_code &error) {
  std::string ip;
  int port = 0;

  if (error) {
    LOG_ERROR("OnAccept - manager(%s) error(%d) message(%s)", _objectName.c_str(), error.value(),
              error.message().c_str());

    if (_acceptSocket != nullptr) {
      _acceptSocket->close();
      _acceptSocket = nullptr;
    }

    if (!_isClosing) {
      PostAccept();
    }

    return;
  }

  if (_isClosing) {
    return;
  }

  if (auto netEvent = _netEvent.lock()) {
    try {
      ip = _acceptSocket->remote_endpoint().address().to_v4().to_string();
      port = _acceptSocket->remote_endpoint().port();
    } catch (const std::exception &e) {
      LOG_ERROR("OnAccept - socket exception - manager(%s) error(%s)", _objectName.c_str(), e.what());

      if (_acceptSocket != nullptr) {
        _acceptSocket->close();
        _acceptSocket = nullptr;
      }

      if (!_isClosing) {
        PostAccept();
      }
      return;
    }

    if (!netEvent->OnAccepted(_objectKey, _acceptSocket, ip, port)) {
      LOG_ERROR("OnAccept - accepted callback fail - manager(%s)", _objectName.c_str());

      if (_acceptSocket != nullptr) {
        LOG_ERROR("OnAccept - socket not null - manager(%s)", _objectName.c_str());
        _acceptSocket->close();
        _acceptSocket = nullptr;
      }
    }
  }

  _acceptSocket = nullptr;
  PostAccept();
}

int TcpManager::FindIndexKey(const std::string &ip, int port) {
  std::lock_guard<std::mutex> lock(_netObjectsLock);

  for (const auto &[indexKey, object] : _netObjects) {
    if (object->GetRemoteIp() == ip && object->GetRemotePort() == port) {
      return indexKey;
    }
  }

  return -1;
}

bool TcpManager::IsConnected(const std::string &ip, int port) { return FindIndexKey(ip, port) != -1; }

std::shared_ptr<TcpObject> TcpManager::Find(int indexKey, bool erase) {
  std::lock_guard<std::mutex> lock(_netObjectsLock);

  if (_netObjects.count(indexKey) != 0) {
    auto object = _netObjects[indexKey];

    if (erase) {
      _netObjects.erase(indexKey);
    }

    return object;
  }

  return nullptr;
}

bool TcpManager::Remove(int indexKey) {
  if (auto object = Find(indexKey, true); object != nullptr) {
    object->PostClose();
    return true;
  }

  return false;
}

void TcpManager::Remove(const std::vector<int> &indexKeys) {
  for (auto indexKey : indexKeys) {
    if (auto object = Find(indexKey, true); object != nullptr) {
      object->PostClose();
    }
  }
}

void TcpManager::RemoveAll() {
  std::lock_guard<std::mutex> lock(_netObjectsLock);

  for (auto &[indexKey, object] : _netObjects) {
    object->Close();
  }

  _netObjects.clear();
}

uint32_t TcpManager::GetCount() const {
  std::lock_guard<std::mutex> lock(_netObjectsLock);
  return static_cast<uint32_t>(_netObjects.size());
}

std::pair<uint64_t, uint64_t> TcpManager::GetTotalTrafficRate(bool init) {
  uint64_t totalSendBitrate = 0;
  uint64_t totalRecvBitrate = 0;

  std::lock_guard<std::mutex> lock(_netObjectsLock);

  for (const auto &[key, object] : _netObjects) {
    auto [sendBitrate, recvBitrate] = object->GetTrafficRate(init);
    totalSendBitrate += sendBitrate;
    totalRecvBitrate += recvBitrate;
  }

  return {totalSendBitrate, totalRecvBitrate};
}

std::pair<uint64_t, uint64_t> TcpManager::GetTrafficRate(int indexKey, bool init) {
  if (auto object = Find(indexKey); object != nullptr) {
    return object->GetTrafficRate(init);
  }

  return {0, 0};
}

uint64_t TcpManager::GetSendTraffic(int indexKey) {
  if (auto object = Find(indexKey); object != nullptr) {
    return object->GetSendTraffic();
  }

  return 0;
}

uint64_t TcpManager::GetSendBufferSize(int indexKey) {
  if (auto object = Find(indexKey); object != nullptr) {
    return object->GetSendBufferSize();
  }

  return 0;
}

uint64_t TcpManager::GetQosSendWaitTime(int indexKey, uint32_t checkBufferSize, uint32_t maxWaitTime,
                                        uint32_t megaPerWait) {
  if (auto object = Find(indexKey); object != nullptr) {
    uint64_t waitTime = object->GetQosSendWaitTime(checkBufferSize, maxWaitTime, megaPerWait);
    return std::min(maxWaitTime, static_cast<uint32_t>(waitTime));
  }

  return 0;
}

std::string TcpManager::GetLocalIP() {
  std::string localIp;
  boost::asio::io_context ioservice;

  boost::asio::ip::tcp::resolver resolver(ioservice);
  boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(), "");

  auto it = resolver.resolve(query);

  while (it != boost::asio::ip::tcp::resolver::iterator()) {
    if (boost::asio::ip::address addr = (it++)->endpoint().address(); addr.is_v4()) {
      localIp = addr.to_string();
      break;
    }
  }

  return localIp;
}

} // namespace Network
