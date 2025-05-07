#include "controller.h"
#include <iostream>

using json = nlohmann::json;

//====================================================================================================
// Send Packet
//====================================================================================================
bool Controller::SendPacket(const BasePacket &packet) {
  if (_clientId.empty()) {
    return false;
  }

  SendEventMsg("message", packet.MakeJson());
  return true;
}

//====================================================================================================
// Send StreamStrat Packet
//====================================================================================================
bool Controller::SendStreamStart(const std::string &streamPath, const std::string &streamApp,
                                 const std::string &streamKey) {
  StreamStartPacket packet(_serviceName, _clientId, streamPath, streamApp, streamKey, _rtmpHost, _rtmpPort);
  return SendPacket(packet);
}

//====================================================================================================
// Recv Event Handler Implement
//====================================================================================================
bool Controller::OnRecv(const std::string &msg) {
  std::cout << "(Websocket) <====== " << msg << std::endl;

  try {
    auto data = json::parse(msg);

    // Service name check
    if (!data.contains("service") || _serviceName != data["service"].get<std::string>()) {
      std::cerr << "Invalid service: " << data["service"] << std::endl;
      return false;
    }

    // Client ID check
    if (!data.contains("clientId") || data["clientId"].get<std::string>().empty() ||
        (!_clientId.empty() && _clientId != data["clientId"].get<std::string>())) {
      std::cerr << "Invalid clientId: " << data["clientId"] << std::endl;
      return false;
    }

    ServiceCmd cmd = GetServiceCmd(data["cmd"].get<std::string>());
    switch (cmd) {
    case ServiceCmd::Connected:
      _clientId = data["clientId"].get<std::string>();
      break;
    case ServiceCmd::VaildateStream: {
      ValidateStreamPacket packet;
      if (!packet.Create(data)) {
        std::cerr << "Failed to parse ValidateStreamPacket" << std::endl;
        return false;
      }

      return OnVaildateStream(packet);
    }
    case ServiceCmd::Error: {
      ErrorPacket packet;
      if (packet.Create(data)) {
        return OnError(packet);
      } else {
        std::cerr << "Failed to parse ErrorPacket" << std::endl;
        return false;
      }
    }
    default:
      std::cerr << "Unknown command received: " << data["cmd"] << std::endl;
      return false;
    }
  } catch (const std::exception &e) {
    std::cerr << "WebSocket parse fail: " << e.what() << std::endl;
    return false;
  }

  return true;
}

//====================================================================================================
// Validate Stream Handler
//====================================================================================================
bool Controller::OnVaildateStream(const ValidateStreamPacket &packet) {
  if (packet.mediaId.empty() || packet.streamPath.empty()) {
    return false;
  }

  auto event = _event.lock();
  if (!event) {
    return false;
  }

  if (packet.isValid) {
    event->OnControllerStreamStart(packet.streamPath, packet.mediaId);
  } else {
    event->OnControllerStreamStop(packet.streamPath);
  }

  return true;
}

//====================================================================================================
// Error Handler
//====================================================================================================
bool Controller::OnError(const ErrorPacket &packet) {
  std::cerr << "Error received: " << packet.error << ", details: " << packet.details << std::endl;
  return true;
}
