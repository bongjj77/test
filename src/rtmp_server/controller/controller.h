#pragma once
#include "controller_packet.hpp"
#include "network/websocket_client.hpp"
#include <memory>
#include <string>

const std::string ServiceName = "rtmp-server";

//====================================================================================================
// Controller Evnet Interface
//====================================================================================================
class ControllerEvent {
public:
  virtual ~ControllerEvent() = default;

  virtual void OnControllerStreamStart(const std::string &streamPath, const std::string &mediaId) = 0;
  virtual void OnControllerStreamStop(const std::string &streamPath) = 0;
};

//====================================================================================================
// Controller (WebSocket)
//====================================================================================================
class Controller : public WebSocketClient {
public:
  Controller(const std::string &host, int port, const std::shared_ptr<net::io_context> &ioc,
             const std::string &rtmpHost, int rtmpPort, const std::shared_ptr<ControllerEvent> &event)
      : WebSocketClient(host, port, ioc), _rtmpHost(rtmpHost), _rtmpPort(rtmpPort), _event(event),
        _serviceName(ServiceName) {}

  bool SendPacket(const BasePacket &packet);
  bool SendStreamStart(const std::string &streamPath, const std::string &streamApp, const std::string &streamKey);

protected:
  void OnConnected() override {};
  void OnSend(std::size_t sendSize) override {};
  bool OnRecv(const std::string &msg) override;
  bool OnVaildateStream(const ValidateStreamPacket &packet);
  bool OnError(const ErrorPacket &packet);

private:
  std::weak_ptr<ControllerEvent> _event;
  std::string _clientId;
  const std::string _serviceName;
  std::string _rtmpHost;
  int _rtmpPort;
};
