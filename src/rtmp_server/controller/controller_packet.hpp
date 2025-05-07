#pragma once
#include <common/json/json.hpp>
#include <iostream>
#include <string>

using json = nlohmann::json;

//====================================================================================================
// Command Defile
//====================================================================================================
enum class ServiceCmd { Connected, Error, StreamStart, VaildateStream, Unknown };

/**
 * String to command
 */
inline ServiceCmd GetServiceCmd(const std::string &cmd) {
  if (cmd == "connected")
    return ServiceCmd::Connected;
  else if (cmd == "stream-start")
    return ServiceCmd::StreamStart;
  else if (cmd == "vaildate-stream")
    return ServiceCmd::VaildateStream;
  else if (cmd == "error")
    return ServiceCmd::Error;
  return ServiceCmd::Unknown;
}

/**
 * Command to string
 */
inline std::string GetServiceString(ServiceCmd cmd) {
  switch (cmd) {
  case ServiceCmd::Connected:
    return "connected";
  case ServiceCmd::StreamStart:
    return "stream-start";
  case ServiceCmd::VaildateStream:
    return "vaildate-stream";
  case ServiceCmd::Error:
    return "error";
  default:
    return "unknown";
  }
}

//====================================================================================================
// Base Packet
//====================================================================================================
class BasePacket {
public:
  std::string service;
  std::string clientId;
  ServiceCmd cmd;

  virtual ~BasePacket() = default;

  virtual bool Create(const json &data) {
    if (!data.contains("service") || !data.contains("clientId") || !data.contains("cmd")) {
      std::cerr << "Invalid message structure - missing fields" << std::endl;
      return false;
    }

    service = data["service"].get<std::string>();
    clientId = data["clientId"].get<std::string>();
    cmd = GetServiceCmd(data["cmd"].get<std::string>());
    return true;
  }

  virtual json MakeJson() const {
    return json{{"service", service}, {"clientId", clientId}, {"cmd", GetServiceString(cmd)}};
  }
};

//====================================================================================================
// Connected Packet
//====================================================================================================
class ConnectedPacket : public BasePacket {
public:
  bool Create(const json &data) override {
    if (!BasePacket::Create(data))
      return false;
    cmd = ServiceCmd::Connected;
    return true;
  }
};

//====================================================================================================
// Stream Start Packet
//====================================================================================================
class StreamStartPacket : public BasePacket {
public:
  std::string streamPath;
  std::string streamApp;
  std::string streamKey;
  std::string rtmpHost;
  int rtmpPort;

  StreamStartPacket(const std::string &service, const std::string &clientId, const std::string &streamPath,
                    const std::string &streamApp, const std::string &streamKey, const std::string &rtmpHost,
                    int rtmpPort) {
    this->service = service;
    this->clientId = clientId;
    this->cmd = ServiceCmd::StreamStart;
    this->streamPath = streamPath;
    this->streamApp = streamApp;
    this->streamKey = streamKey;
    this->rtmpHost = rtmpHost;
    this->rtmpPort = rtmpPort;
  }

  json MakeJson() const override {
    auto data = BasePacket::MakeJson();
    data["streamPath"] = streamPath;
    data["streamApp"] = streamApp;
    data["streamKey"] = streamKey;
    data["rtmpHost"] = rtmpHost;
    data["rtmpPort"] = rtmpPort;
    return data;
  }
};

//====================================================================================================
// Validate Stream Packet
//====================================================================================================
class ValidateStreamPacket : public BasePacket {
public:
  bool isValid;
  std::string streamPath;
  std::string mediaId;

  bool Create(const json &data) override {
    if (!BasePacket::Create(data))
      return false;

    if (!data.contains("isValid") || !data.contains("streamPath") || !data.contains("mediaId")) {
      std::cerr << "Invalid ValidateStreamPacket structure - missing fields" << std::endl;
      return false;
    }

    isValid = data["isValid"].get<bool>();
    mediaId = data["mediaId"].get<std::string>();
    streamPath = data["streamPath"].get<std::string>();

    return true;
  }

  json MakeJson() const override {
    auto data = BasePacket::MakeJson();
    data["isValid"] = isValid;
    data["streamPath"] = streamPath;
    data["mediaId"] = mediaId;
    return data;
  }
};

//====================================================================================================
// Error Packet
//====================================================================================================
class ErrorPacket : public BasePacket {
public:
  std::string error;
  std::string details;

  bool Create(const json &data) override {
    if (!BasePacket::Create(data))
      return false;

    if (!data.contains("error")) {
      std::cerr << "Invalid ErrorPacket structure - missing fields" << std::endl;
      return false;
    }

    error = data["error"].get<std::string>();
    if (data.contains("details")) {
      details = data["details"].get<std::string>();
    }

    return true;
  }

  json MakeJson() const override {
    auto data = BasePacket::MakeJson();
    data["error"] = error;
    if (!details.empty()) {
      data["details"] = details;
    }
    return data;
  }
};
