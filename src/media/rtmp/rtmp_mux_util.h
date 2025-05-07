#pragma once
#include "rtmp_define.h"
#include <memory>
#include <tuple>
#include <vector>

#pragma pack(1)
//====================================================================================================
// RtmpMuxMsgHeader
//====================================================================================================
struct RtmpMuxMsgHeader {
public:
    RtmpMuxMsgHeader() = default;

    RtmpMuxMsgHeader(uint32_t chunk_stream_id_, uint32_t timestamp_, uint8_t type_id_, uint32_t stream_id_, uint32_t body_size_)
        : chunkStreamId(chunk_stream_id_), timestamp(timestamp_), typeId(type_id_), streamId(stream_id_), bodySize(body_size_) {}

public:
    uint32_t chunkStreamId = 0;
    uint32_t timestamp = 0;
    uint8_t typeId = 0;
    uint32_t streamId = 0;
    uint32_t bodySize = 0;
};
#pragma pack()
//====================================================================================================
// RtmpMuxUtil
//====================================================================================================
class RtmpMuxUtil {
public:
    RtmpMuxUtil() = default;
    virtual ~RtmpMuxUtil() = default;

public:
    static uint8_t ReadInt8(const uint8_t* data);
    static uint16_t ReadInt16(const uint8_t* data);
    static uint32_t ReadInt24(const uint8_t* data);
    static uint32_t ReadInt32(const uint8_t* data);
    static int ReadInt32LE(const uint8_t* data);

    static int WriteInt8(uint8_t* output, uint8_t value);
    static int WriteInt16(uint8_t* output, int16_t value);
    static int WriteInt24(uint8_t* output, int value);
    static int WriteInt32(uint8_t* output, int value);
    static int WriteInt32LE(uint8_t* output, int value);

    static bool WriteInt8(const std::shared_ptr<std::vector<uint8_t>>& output, uint8_t value);
    static bool WriteInt16(const std::shared_ptr<std::vector<uint8_t>>& output, int16_t value);
    static bool WriteInt24(const std::shared_ptr<std::vector<uint8_t>>& output, int value);
    static bool WriteInt32(const std::shared_ptr<std::vector<uint8_t>>& output, int value);
    static bool WriteInt32LE(const std::shared_ptr<std::vector<uint8_t>>& output, int value);

    static int GetBasicHeaderSizeByRawData(uint8_t nData);
    static int GetBasicHeaderSizeByChunkStreamID(uint32_t chunkStreamId);
    static int GetChunkHeaderSize(const uint8_t* rawData, int rawDataSize);
    static std::tuple<std::shared_ptr<Rtmp::ChunkHeader>, int, bool> GetChunkHeader(const uint8_t* rawData, int rawDataSize);
    static std::shared_ptr<std::vector<uint8_t>> MakeChunkBasicHeader(Rtmp::ChunkFormat chunk_format, uint32_t chunkStreamId);
    static std::shared_ptr<std::vector<uint8_t>> MakeChunkRawHeader(const std::shared_ptr<Rtmp::ChunkHeader>& chunkHeader, bool isEextend);
    static int GetChunkDataRawSize(int chunkSize, uint32_t chunkStreamId, int chunkDataSize, bool isEextend);
    static std::shared_ptr<std::vector<uint8_t>> MakeChunkRawData(int chunkSize, uint32_t chunkStreamId, const std::shared_ptr<std::vector<uint8_t>>& chunkData, bool isEextend, uint32_t time);
};