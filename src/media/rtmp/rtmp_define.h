#pragma once
#include "../../common/common_header.h"
#include <memory>
#include <string>
#include <vector>
#pragma pack(1)

namespace Rtmp {

// HANDSHAKE
constexpr int HandshakeVersion = 0x03;
constexpr int HandshakePacketSize = 1536;

// CHUNK
constexpr int ChunkBasicHeaderSizeMax = 3;
constexpr int ChunkMessageHeaderSizeMax = 11 + 4; // header + extended timestamp
constexpr int PacketHeaderSizeMax = ChunkBasicHeaderSizeMax + ChunkMessageHeaderSizeMax;
constexpr int ChunkBasicFormatTypeMask = 0xc0;
constexpr int ChunkBasicChunkStreamIdMask = 0x3f;

enum class ChunkFormat : uint8_t {
  Type_0 = 0x00,
  Type_1 = 0x40,
  Type_2 = 0x80,
  Type_3 = 0xc0,
  Unknown = 0xff,
};

struct ChunkTypeSize {
  static constexpr int Type_0 = 11;
  static constexpr int Type_1 = 7;
  static constexpr int Type_2 = 3;
  static constexpr int Type_3 = 0;
};

constexpr int ExtendFormat = 0x00ffffff;
constexpr int ExtendTimestampSize = 4;

// CHUNK STREAM ID
enum class ChunkStreamType {
  Urgent = 2,
  Control = 3,
  Stream = 4,
  Media = 8,
};

// MESSAGE ID
enum class MsgType {
  SetChunkSize = 1,
  AbortMsg = 2,
  Ack = 3,
  UserControlMsg = 4,
  WindowAckSize = 5,
  SetPeerBandWidth = 6,
  AudioMsg = 8,
  VideoMsg = 9,
  Amf3DataMsg = 15,
  Amf3CmdMsg = 17,
  Amf0DataMsg = 18,
  Amf0CmdMsg = 20,
  AggregateMsg = 22,
};

// USER CONTROL MESSAGE ID
enum class UserControlMsgType {
  StreamBegin = 0,
  StreamEof = 1,
  StreamDry = 2,
  SetBufferLength = 3,
  StreamIsRecorded = 4,
  PingRequest = 6,
  PingResponse = 7,
  BufferEmpty = 31,
  BufferReady = 32,
};

struct Cmd {
  static inline const std::string Connect = "connect";
  static inline const std::string CreateStream = "createStream";
  static inline const std::string ReleaseStream = "releaseStream";
  static inline const std::string DeleteStream = "deleteStream";
  static inline const std::string CloseStream = "closeStream";
  static inline const std::string Play = "play";
  static inline const std::string OnStatus = "onStatus";
  static inline const std::string Publish = "publish";
  static inline const std::string FcPublish = "FCPublish";
  static inline const std::string FcUnpublish = "FCUnpublish";
  static inline const std::string OnFcpublish = "onFCPublish";
  static inline const std::string OnUnfcpublish = "onFCUnpublish";
  static inline const std::string Close = "close";
  static inline const std::string SetChallenge = "setChallenge";
  static inline const std::string SetDataFrame = "@setDataFrame";
  static inline const std::string OnClientLogin = "onClientLogin";
  static inline const std::string OnMetaData = "onMetaData";
  static inline const std::string Result = "_result";
  static inline const std::string Error = "_error";
  static inline const std::string OnBwDone = "onBWDone";
  static inline const std::string Ping = "ping";
  static inline const std::string Pong = "pong";
};

// RtmpStream/RtmpPublish
constexpr int H264IFrameType = 0x17;
constexpr int H264PFrameType = 0x27;
constexpr int DefaultAckSize = 2500000;
constexpr int DefaultPeerBandWidth = 2500000;
constexpr int DefaultChunkSize = 128;

constexpr int VideoDataMinSize = 5; // control(1) + sequence(1) + offset time(3)

constexpr int AudioControlHeaderIndex = 0;
constexpr int AacAudioSequenceHeaderIndex = 1;
constexpr int AacAudioDataMinSize = 2; // control(1) + sequence(1)
constexpr int AacAudioFrameIndex = 2;
constexpr int SpeexAudioFrameIndex = 1;
constexpr int Mp3AudioFrameIndex = 1;
constexpr int SpsPpsMinDataSize = 14;

constexpr int VideoControlHeaderIndex = 0;
constexpr int VideoSequenceHeaderIndex = 1;
constexpr int VideoCtsIndex = 2;
constexpr int VideoFrameIndex = 5;

constexpr int DefaultPort = 1935;

struct Type0 {
  uint32_t timestamp;
  uint32_t bodySize;
  uint8_t typeId;
  uint32_t streamId;
};

struct Type1 {
  uint32_t timestampDelta;
  uint32_t bodySize;
  uint8_t typeId;
};

struct Type2 {
  uint32_t timestampDelta;
};

//====================================================================================================
// ChunkHeader
//====================================================================================================
struct ChunkHeader {
  ChunkHeader() { Init(); }

  void Init() {
    basicHeader.formatType = ChunkFormat::Unknown;
    basicHeader.chunkStreamId = 0;
    type_0 = {0, 0, 0, 0};
  }

  struct BasicHeader {
    ChunkFormat formatType;
    uint32_t chunkStreamId; // 0, 1, 2 are reserved
  } basicHeader;

  union {
    Type0 type_0;
    Type1 type_1;
    Type2 type_2;
  };
};

//===============================================================================================
// Handshake State
//===============================================================================================
enum class HandshakeState {
  Ready = 0,
  C0,
  C1,
  S0,
  S1,
  S2,
  C2,
  Complete,
};

constexpr int MaxPacketSize = 20 * 1024 * 1024; // 20M

//===============================================================================================
// Frame
//===============================================================================================
struct Frame {
  Frame(uint64_t timestamp_, std::shared_ptr<std::vector<uint8_t>> data_)
      : timestamp(timestamp_), data(std::move(data_)) {}

  explicit Frame(const std::shared_ptr<Frame> &frame)
      : timestamp(frame->timestamp), data(std::make_shared<std::vector<uint8_t>>(*frame->data)) {}

  uint64_t timestamp; // dts or pts
  std::shared_ptr<std::vector<uint8_t>> data;
};

} // namespace Rtmp
#pragma pack()
