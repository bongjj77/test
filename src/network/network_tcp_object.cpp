#include "network_tcp_object.h"
#include "network_manager.h"
#include <iostream>
#include <stdarg.h>

namespace Network {

TcpObject::TcpObject() {}

TcpObject::~TcpObject() {
  SendQueueRelease();
  _socket = nullptr;
}

bool TcpObject::Create(const std::shared_ptr<NetTcpParam> &param) {
  if (!param->socket) {
    return false;
  }

  _socket = param->socket;
  _objectKey = param->objectKey;
  _objectName = param->objectName;
  _netEvent = param->netEvent;
  _createTime = time(nullptr);
  _ip = _socket->remote_endpoint().address().to_string();
  _port = _socket->remote_endpoint().port();

  return true;
}

bool TcpObject::IsOpened() const { return _socket->is_open(); }

boost::asio::io_context &TcpObject::GetIoContext() {
  return static_cast<boost::asio::io_context &>(_socket->get_executor().context());
}

void TcpObject::AsyncRecv() {
  if (_isClosing || !IsOpened()) {
    return;
  }

  _socket->async_read_some(
      boost::asio::buffer(*_recvBuffer),
      [self = shared_from_this()](const boost::system::error_code &error, std::size_t readSize) {
        self->OnReceive(error, readSize);
      });
}

void TcpObject::AsyncSend(const std::shared_ptr<SendData> &sendData) {
  if (_isClosing || !IsOpened()) {
    return;
  }

  sendData->sendTime = time(nullptr);

  boost::asio::async_write(*_socket, boost::asio::buffer(*(sendData->data)),
                           [self = shared_from_this()](const boost::system::error_code &error,
                                                       std::size_t writeSize) { self->OnSend(error, writeSize); });
}

void TcpObject::SendQueueRelease() {
  std::lock_guard<std::mutex> send_data_lock(_sendQueueLock);
  _sendQueue.clear();
}

bool TcpObject::Start() {
  if (!IsOpened()) {
    return false;
  }

  AsyncRecv();
  return true;
}

void TcpObject::PostClose() {
  _isClosing = true;

  if (_postCloseTimer) {
    return;
  }

  _postCloseTimer = std::make_shared<boost::asio::steady_timer>(GetIoContext());
  _postCloseTimer->expires_from_now(std::chrono::milliseconds(_postCloseTimeInterval));
  _postCloseTimer->async_wait([self = shared_from_this(), timer = _postCloseTimer,
                               interval = _postCloseTimeInterval](const boost::system::error_code &error) {
    self->OnNetworkTimer(error, timer, Timer::PostClose, interval);
  });
}

void TcpObject::Close() {
  _isClosing = true;
  PostCloseTimerProc();
}

bool TcpObject::PostSend(std::shared_ptr<std::vector<uint8_t>> data, bool isDataCopy) {
  if (!data || data->empty()) {
    LOG_ERROR("[%s] TcpObject::PostSend - Param Fail - key(%d) ip(%s)", _objectName.c_str(), _indexKey, _ip.c_str());
    return false;
  }

  if (_isClosing) {
    LOG_INFO("[%s] TcpObject::PostSend - Closing Return - key(%d) ip(%s)", _objectName.c_str(), _indexKey, _ip.c_str());
    return false;
  }

  auto sendData = std::make_shared<SendData>(isDataCopy, data);

  std::lock_guard<std::mutex> send_data_lock(_sendQueueLock);
  _sendQueue.emplace_back(sendData);
  _sendBufferSize += data->size();

  if (_sendQueue.size() == 1) {
    AsyncSend(_sendQueue.front());
  }

  return true;
}

void TcpObject::OnReceive(const boost::system::error_code &error, size_t dataSize) {
  if (error) {
    _networkError = true;
    if (!_isClosing) {
      _isClosing = true;
      if (auto netEvent = _netEvent.lock()) {
        netEvent->OnClosed(_objectKey, _indexKey, _ip, _port);
      }
    }
    return;
  }

  if (dataSize == 0) {
    LOG_WARN("Network recv data size zero");
  }

  if (_isClosing || _networkError) {
    return;
  }

  _lastRecvTime = time(nullptr);

  if (_restData && !_restData->empty()) {
    _recvData = _restData;
    _restData = nullptr;
    _recvData->insert(_recvData->end(), _recvBuffer->begin(), _recvBuffer->begin() + dataSize);
  } else {
    _recvData = std::make_shared<std::vector<uint8_t>>(_recvBuffer->begin(), _recvBuffer->begin() + dataSize);
  }

  int procSize = RecvHandler(_recvData);

  if (procSize < 0 || static_cast<size_t>(procSize) > _recvData->size()) {
    LOG_ERROR("[%s] TcpObject::OnReceive - RecvHandler - key(%d) ip(%s) Result(%d)", _objectName.c_str(), _indexKey,
              _ip.c_str(), procSize);

    if (!_isClosing) {
      _isClosing = true;
      if (auto netEvent = _netEvent.lock()) {
        netEvent->OnClosed(_objectKey, _indexKey, _ip, _port);
      }
    }
    return;
  }

  if (int restDataSize = static_cast<int>(_recvData->size()) - procSize; restDataSize > 0) {
    _restData = std::make_shared<std::vector<uint8_t>>(_recvData->data() + procSize,
                                                       _recvData->data() + procSize + restDataSize);
  }

  AsyncRecv();
  _recvTraffic += procSize;
}

void TcpObject::OnSend(const boost::system::error_code &error, size_t dataSize) {
  if (error) {
    _networkError = true;
    if (!_isClosing) {
      _isClosing = true;
      if (auto netEvent = _netEvent.lock()) {
        netEvent->OnClosed(_objectKey, _indexKey, _ip, _port);
      }
    }
    return;
  }

  if (_isClosing || _networkError) {
    return;
  }

  std::lock_guard<std::mutex> send_data_lock(_sendQueueLock);

  if (!_sendQueue.empty()) {
    _sendCompleteTime = time(nullptr);
    _sendBufferSize -= dataSize;
    _sendQueue.pop_front();

    if (!_sendQueue.empty()) {
      AsyncSend(_sendQueue.front());
    }
  }

  if (_isSendCompletedClose && _sendQueue.empty()) {
    _isClosing = true;
    if (auto netEvent = _netEvent.lock()) {
      netEvent->OnClosed(_objectKey, _indexKey, _ip, _port);
    }
  }

  _sendTraffic += dataSize;
}

void TcpObject::SetRecvTimeout(uint32_t timeout) {
  if (_timeoutTimer) {
    _timeoutTimer->cancel();
    _timeoutTimer = nullptr;
  }

  _timeoutTimer = std::make_shared<boost::asio::steady_timer>(GetIoContext());
  _timeout = timeout;
  SetNetworkTimer(_timeoutTimer, Timer::TimeoutCheck, _timeout * 1000 / 2);
}

void TcpObject::SetNetworkTimer(std::shared_ptr<boost::asio::steady_timer> networkTimer, Timer id, int interval) {
  if (_isClosing) {
    return;
  }

  networkTimer->expires_from_now(std::chrono::milliseconds(interval));
  networkTimer->async_wait(
      [self = shared_from_this(), timer = networkTimer, id, interval](const boost::system::error_code &error) {
        if (!error) {
          self->OnNetworkTimer(error, timer, id, interval);
        } else {
          // Handle error
        }
      });
}

void TcpObject::OnNetworkTimer(const boost::system::error_code &error,
                               std::shared_ptr<boost::asio::steady_timer> networkTimer, Timer id, int interval) {
  if (_isClosing) {
    if (id == Timer::PostClose) {
      PostCloseTimerProc();
    }
    return;
  }

  if (error) {
    return;
  }

  switch (id) {
  case Timer::KeepaliveSend:
    if (_keepaliveSendCallback && !(this->*_keepaliveSendCallback)()) {
      return;
    }
    SetNetworkTimer(networkTimer, id, interval);
    break;
  case Timer::TimeoutCheck:
    if ((time(nullptr) - _lastRecvTime) > _timeout) {
      LOG_INFO("Network timeout - object(%s) key(%d) ip(%s) gap(%ld)", _objectName.c_str(), _indexKey, _ip.c_str(),
               time(nullptr) - _lastRecvTime);
      if (!_isClosing) {
        if (auto netEvent = _netEvent.lock()) {
          netEvent->OnClosed(_objectKey, _indexKey, _ip, _port);
        }
      }
      return;
    }
    SetNetworkTimer(networkTimer, id, interval);
    break;
  default:
    LOG_WARN("Network timer - unknown id - object(%s) key(%d) ip(%s) id(%d)", _objectName.c_str(), _indexKey,
             _ip.c_str(), static_cast<int>(id));
  }
}

bool TcpObject::PostCloseTimerProc() {
  if (_keepaliveSendTimer) {
    _keepaliveSendTimer->cancel();
  }
  if (_timeoutTimer) {
    _timeoutTimer->cancel();
  }
  if (_postCloseTimer) {
    _postCloseTimer->cancel();
  }

  if (_socket->is_open()) {
    boost::system::error_code ec;
    _socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    _socket->close(ec);
  }

  return true;
}

std::pair<uint64_t, uint64_t> TcpObject::GetTrafficRate(bool init) {
  uint64_t curTime = GetCurrentMs();

  if (uint64_t checkGap = curTime - _trafficCheckTime; checkGap >= 1000) {
    auto sendBitrate = _sendTraffic * 8 * 1000 / checkGap;
    auto recvBitrate = _recvTraffic * 8 * 1000 / checkGap;

    if (init) {
      _trafficCheckTime = curTime;
      _sendTraffic = 0;
      _recvTraffic = 0;
    }

    return {sendBitrate, recvBitrate};
  }

  return {0, 0};
}

uint64_t TcpObject::GetQosSendWaitTime(uint32_t checkBufferSize, uint32_t maxWaitTime, uint32_t megaPerWait) {
  if (_sendBufferSize < checkBufferSize) {
    return 0;
  }

  uint64_t waitTime = (_sendBufferSize / (1024 * 1024) * megaPerWait);
  return std::min(static_cast<uint64_t>(maxWaitTime), waitTime);
}

} // namespace Network
