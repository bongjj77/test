#pragma once
#include "common/json/json.hpp"
#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace websocket = beast::websocket;
using json = nlohmann::json;

class WebSocketClient : public std::enable_shared_from_this<WebSocketClient> {
public:
  WebSocketClient(const std::string &uri, int port, const std::shared_ptr<net::io_context> &ioc)
      : _ioc(ioc), _uri(uri), _port(port), _resolver(*ioc), _connected(false) {
    _ws = std::make_unique<websocket::stream<tcp::socket>>(*ioc);
  }

  virtual ~WebSocketClient() = default;

  virtual void Connect() {
    auto self = shared_from_this();

    // Asynchronously connect to the resolved endpoint
    auto connected = [self](beast::error_code ec, tcp::resolver::iterator) {
      if (ec) {
        std::cerr << "Connect failed: " << ec.message() << std::endl;
        self->Reconnect();
        return;
      }

      // Perform the WebSocket handshake
      self->_ws->async_handshake(self->_uri, "/", [self](beast::error_code ec) {
        if (ec) {
          std::cerr << "Handshake failed: " << ec.message() << std::endl;
          self->Reconnect();
          return;
        }

        self->_connected = true;
        self->AsyncRead();
        self->OnConnected();
      });
    };

    auto resolved = [self, connected](beast::error_code ec, tcp::resolver::results_type results) {
      if (ec) {
        std::cerr << "Resolve failed: " << ec.message() << std::endl;
        self->Reconnect();
        return;
      }

      net::async_connect(self->_ws->next_layer(), results.begin(), results.end(), connected);
    };

    // Resolve the host and port
    _resolver.async_resolve(_uri, std::to_string(_port), resolved);
  }

  virtual void Reconnect() {

    if (_ws->is_open()) {
      auto self = shared_from_this();
      auto closed = [self](beast::error_code ec) {
        if (ec) {
          std::cerr << "Close failed: " << ec.message() << std::endl;
        }
        self->_ws = std::make_unique<websocket::stream<tcp::socket>>(*(self->_ioc));
        std::this_thread::sleep_for(std::chrono::seconds(5));
        self->Connect();
      };

      _ws->async_close(websocket::close_code::normal, closed);
    } else {
      _ws = std::make_unique<websocket::stream<tcp::socket>>(*_ioc);
      std::this_thread::sleep_for(std::chrono::seconds(5));
      Connect();
    }
  }

  virtual void AsyncRead() {
    auto read = [self = shared_from_this()](beast::error_code ec, std::size_t readSize) {
      if (ec) {
        self->OnRecvError(ec);
        return;
      }
      std::string msg(net::buffers_begin(self->_buffer.data()), net::buffers_begin(self->_buffer.data()) + readSize);
      self->_buffer.consume(readSize);

      if (self->OnRecv(msg)) {
        self->AsyncRead();
      } else {
        self->Close();
      }
    };

    _ws->async_read(_buffer, read);
  }

  virtual void SendMsg(const std::string &msg) {
    std::cout << "(Websocket) =====> " << msg << std::endl;

    auto writed = [self = shared_from_this()](beast::error_code ec, std::size_t writeSize) {
      if (ec) {
        self->OnSendError(ec);
      } else {
        self->OnSend(writeSize);
      }
    };

    _ws->async_write(net::buffer(msg), writed);
  }

  virtual void SendEventMsg(const std::string &event, const json &data) {
    json meg = {{"event", event}, {"data", data}};
    SendMsg(meg.dump());
  }

  bool IsConnected() const { return _connected; }

protected:
  virtual void OnConnected() = 0;
  virtual bool OnRecv(const std::string &msg) = 0;
  virtual void OnSend(std::size_t sendSize) = 0;
  virtual void OnClose(beast::error_code ec) {
    if (ec) {
      std::cerr << "Close failed: " << ec.message() << std::endl;
    }
    _connected = false;
    Reconnect();
  }

  virtual void OnRecvError(beast::error_code ec) {
    std::cerr << "Read failed: " << ec.message() << std::endl;
    _connected = false;
    Reconnect();
  }

  virtual void OnSendError(beast::error_code ec) {
    std::cerr << "Write failed: " << ec.message() << std::endl;
    _connected = false;
    Reconnect();
  }

  void Close() {
    auto self = shared_from_this();
    _ws->async_close(websocket::close_code::normal, [self](beast::error_code ec) {
      if (ec) {
        std::cerr << "Close failed: " << ec.message() << std::endl;
      }
      self->_connected = false;
    });
  }

  std::shared_ptr<net::io_context> _ioc;
  std::string _uri;
  int _port;
  tcp::resolver _resolver;
  std::unique_ptr<websocket::stream<tcp::socket>> _ws;
  beast::flat_buffer _buffer;
  bool _connected;
};
