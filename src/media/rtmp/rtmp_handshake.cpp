#include "rtmp_handshake.h"

//====================================================================================================
// Fill Handshake Packet
//====================================================================================================
void RtmpHandshake::FillHandshakePacket(std::vector<uint8_t> &buffer) {
  uint8_t *dataPos = buffer.data();

  // S0 / C0
  dataPos[0] = Rtmp::HandshakeVersion;

  // Current time
  uint32_t curTime = static_cast<uint32_t>(std::time(nullptr));
  dataPos[1] = static_cast<uint8_t>(curTime >> 24 & 0xFF);
  dataPos[2] = static_cast<uint8_t>(curTime >> 16 & 0xFF);
  dataPos[3] = static_cast<uint8_t>(curTime >> 8 & 0xFF);
  dataPos[4] = static_cast<uint8_t>(curTime & 0xFF);

  // Random fill
  std::mt19937 random_number{std::random_device{}()};
  for (int index = 9; index < Rtmp::HandshakePacketSize; ++index) {
    dataPos[index] = static_cast<uint8_t>(random_number() % 256);
  }
}

//====================================================================================================
// Make S0 S1 S2
//====================================================================================================
std::shared_ptr<std::vector<uint8_t>> RtmpHandshake::MakeS0_S1_S2(const uint8_t *c1) {
  auto buffer = std::make_shared<std::vector<uint8_t>>(1 + Rtmp::HandshakePacketSize);
  FillHandshakePacket(*buffer);

  // S2: Copy c1
  buffer->insert(buffer->end(), c1, c1 + Rtmp::HandshakePacketSize);

  return buffer;
}

//====================================================================================================
// Make C0 C1
//====================================================================================================
std::shared_ptr<std::vector<uint8_t>> RtmpHandshake::MakeC0_C1() {
  auto buffer = std::make_shared<std::vector<uint8_t>>(1 + Rtmp::HandshakePacketSize);
  FillHandshakePacket(*buffer);

  return buffer;
}

//====================================================================================================
// Make C2
//====================================================================================================
std::shared_ptr<std::vector<uint8_t>> RtmpHandshake::MakeC2(const uint8_t *s1) {
  return std::make_shared<std::vector<uint8_t>>(s1, s1 + Rtmp::HandshakePacketSize);
}