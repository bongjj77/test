#include "rtmp_mux_util.h"

uint8_t RtmpMuxUtil::ReadInt8(const uint8_t *data) { return data[0]; }

uint16_t RtmpMuxUtil::ReadInt16(const uint8_t *data) { return static_cast<uint16_t>((data[0] << 8) | data[1]); }

uint32_t RtmpMuxUtil::ReadInt24(const uint8_t *data) {
  return static_cast<uint32_t>((data[0] << 16) | (data[1] << 8) | data[2]);
}

uint32_t RtmpMuxUtil::ReadInt32(const uint8_t *data) {
  return static_cast<uint32_t>((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);
}

int RtmpMuxUtil::ReadInt32LE(const uint8_t *data) {
  int result;
  std::memcpy(&result, data, sizeof(int));
  return result;
}

int RtmpMuxUtil::WriteInt8(uint8_t *output, uint8_t value) {
  output[0] = value;
  return sizeof(uint8_t);
}

int RtmpMuxUtil::WriteInt16(uint8_t *output, int16_t value) {
  output[1] = static_cast<uint8_t>(value & 0xff);
  output[0] = static_cast<uint8_t>(value >> 8);
  return sizeof(int16_t);
}

int RtmpMuxUtil::WriteInt24(uint8_t *output, int value) {
  output[2] = static_cast<uint8_t>(value & 0xff);
  output[1] = static_cast<uint8_t>(value >> 8);
  output[0] = static_cast<uint8_t>(value >> 16);
  return 3;
}

int RtmpMuxUtil::WriteInt32(uint8_t *output, int value) {
  output[3] = static_cast<uint8_t>(value & 0xff);
  output[2] = static_cast<uint8_t>(value >> 8);
  output[1] = static_cast<uint8_t>(value >> 16);
  output[0] = static_cast<uint8_t>(value >> 24);
  return sizeof(int);
}

int RtmpMuxUtil::WriteInt32LE(uint8_t *output, int value) {
  std::memcpy(output, &value, sizeof(int));
  return sizeof(int);
}

bool RtmpMuxUtil::WriteInt8(const std::shared_ptr<std::vector<uint8_t>> &output, uint8_t value) {
  output->push_back(value);
  return true;
}

bool RtmpMuxUtil::WriteInt16(const std::shared_ptr<std::vector<uint8_t>> &output, int16_t value) {
  output->push_back(value >> 8);
  output->push_back(value & 0xff);
  return true;
}

bool RtmpMuxUtil::WriteInt24(const std::shared_ptr<std::vector<uint8_t>> &output, int value) {
  output->push_back(value >> 16);
  output->push_back(value >> 8);
  output->push_back(value & 0xff);
  return true;
}

bool RtmpMuxUtil::WriteInt32(const std::shared_ptr<std::vector<uint8_t>> &output, int value) {
  output->push_back(value >> 24);
  output->push_back(value >> 16);
  output->push_back(value >> 8);
  output->push_back(value & 0xff);
  return true;
}

bool RtmpMuxUtil::WriteInt32LE(const std::shared_ptr<std::vector<uint8_t>> &output, int value) {
  output->insert(output->end(), reinterpret_cast<uint8_t *>(&value), reinterpret_cast<uint8_t *>(&value) + sizeof(int));
  return true;
}

//====================================================================================================
// GetBasicHeaderSizeByRawData
//====================================================================================================
int RtmpMuxUtil::GetBasicHeaderSizeByRawData(uint8_t data) {
  switch (data & Rtmp::ChunkBasicChunkStreamIdMask) {
  case 0:
    return 2;
  case 1:
    return 3;
  default:
    return 1;
  }
}

//====================================================================================================
// GetBasicHeaderSizeByChunkStreamID
//====================================================================================================
int RtmpMuxUtil::GetBasicHeaderSizeByChunkStreamID(uint32_t chunkStreamId) {
  if (chunkStreamId >= (64 + 256))
    return 3;
  else if (chunkStreamId >= 64)
    return 2;
  else
    return 1;
}

//====================================================================================================
// GetChunkHeaderSize
//====================================================================================================
int RtmpMuxUtil::GetChunkHeaderSize(const uint8_t *rawData, int rawDataSize) {
  int basicHeaderSize = 0;
  int msgHeaderSize = 0;
  int headerSize = 0;

  if (rawData == nullptr || rawDataSize == 0) {
    return 0;
  }

  auto format = static_cast<Rtmp::ChunkFormat>(rawData[0] & Rtmp::ChunkBasicFormatTypeMask);
  auto chunkStreamId = static_cast<uint32_t>(rawData[0] & Rtmp::ChunkBasicChunkStreamIdMask);

  // basicHeader length
  if (chunkStreamId == 0)
    basicHeaderSize = 2;
  else if (chunkStreamId == 1)
    basicHeaderSize = 3;
  else
    basicHeaderSize = 1;

  // msg header length
  switch (format) {
  case Rtmp::ChunkFormat::Type_0:
    msgHeaderSize = Rtmp::ChunkTypeSize::Type_0;
    break;
  case Rtmp::ChunkFormat::Type_1:
    msgHeaderSize = Rtmp::ChunkTypeSize::Type_1;
    break;
  case Rtmp::ChunkFormat::Type_2:
    msgHeaderSize = Rtmp::ChunkTypeSize::Type_2;
    break;
  case Rtmp::ChunkFormat::Type_3:
    msgHeaderSize = Rtmp::ChunkTypeSize::Type_3;
    break;
  default:
    return -1;
  }

  headerSize = basicHeaderSize + msgHeaderSize;

  if (rawDataSize < headerSize) {
    return 0;
  }

  // Extended Timestamp 확인(Type3는 확인 못함 - 이전 정보를 기반으로 이후에 확인)
  if (format != Rtmp::ChunkFormat::Type_3 && RtmpMuxUtil::ReadInt24(rawData + basicHeaderSize) == Rtmp::ExtendFormat) {
    headerSize += Rtmp::ExtendTimestampSize;

    if (rawDataSize < headerSize) {
      return 0;
    }
  }

  return headerSize;
}

//====================================================================================================
// GetChunkHeader
//====================================================================================================
std::tuple<std::shared_ptr<Rtmp::ChunkHeader>, int, bool> RtmpMuxUtil::GetChunkHeader(const uint8_t *rawData,
                                                                                      int rawDataSize) {
  if (!rawData || rawDataSize == 0)
    return {nullptr, 0, false};

  int basicHeaderSize = GetBasicHeaderSizeByRawData(rawData[0]);
  int chunkHeaderSize = GetChunkHeaderSize(rawData, rawDataSize);
  if (chunkHeaderSize <= 0)
    return {nullptr, chunkHeaderSize, false};

  auto chunkHeader = std::make_shared<Rtmp::ChunkHeader>();
  bool isEextend = false;

  auto format = static_cast<Rtmp::ChunkFormat>(rawData[0] & Rtmp::ChunkBasicFormatTypeMask);
  auto chunkStreamId = static_cast<uint32_t>(rawData[0] & Rtmp::ChunkBasicChunkStreamIdMask);

  if (chunkStreamId == 0)
    chunkStreamId = 64 + rawData[1];
  else if (chunkStreamId == 1)
    chunkStreamId = 64 + rawData[1] + rawData[2] * 256;

  chunkHeader->basicHeader.formatType = format;
  chunkHeader->basicHeader.chunkStreamId = chunkStreamId;

  const uint8_t *dataPos = rawData + basicHeaderSize;

  switch (format) {
  case Rtmp::ChunkFormat::Type_0:
    chunkHeader->type_0.timestamp = ReadInt24(dataPos);
    chunkHeader->type_0.bodySize = ReadInt24(dataPos + 3);
    chunkHeader->type_0.typeId = dataPos[6];
    chunkHeader->type_0.streamId = ReadInt32LE(dataPos + 7);
    if (chunkHeader->type_0.timestamp == Rtmp::ExtendFormat) {
      chunkHeader->type_0.timestamp = ReadInt32(dataPos + 11);
      isEextend = true;
    }
    break;
  case Rtmp::ChunkFormat::Type_1:
    chunkHeader->type_1.timestampDelta = ReadInt24(dataPos);
    chunkHeader->type_1.bodySize = ReadInt24(dataPos + 3);
    chunkHeader->type_1.typeId = dataPos[6];
    if (chunkHeader->type_1.timestampDelta == Rtmp::ExtendFormat) {
      chunkHeader->type_1.timestampDelta = ReadInt32(dataPos + 7);
      isEextend = true;
    }
    break;
  case Rtmp::ChunkFormat::Type_2:
    chunkHeader->type_2.timestampDelta = ReadInt24(dataPos);
    if (chunkHeader->type_2.timestampDelta == Rtmp::ExtendFormat) {
      chunkHeader->type_2.timestampDelta = ReadInt32(dataPos + 3);
      isEextend = true;
    }
    break;
  case Rtmp::ChunkFormat::Type_3:
    break;
  default:
    break;
  }

  return {chunkHeader, chunkHeaderSize, isEextend};
}

//====================================================================================================
// MakeChunkBasicHeader
//====================================================================================================
std::shared_ptr<std::vector<uint8_t>> RtmpMuxUtil::MakeChunkBasicHeader(Rtmp::ChunkFormat chunk_format,
                                                                        uint32_t chunkStreamId) {
  auto basicHeader = std::make_shared<std::vector<uint8_t>>();
  if (chunkStreamId >= (64 + 256)) {
    basicHeader->push_back(1);
    chunkStreamId -= 64;
    basicHeader->push_back(static_cast<uint8_t>(chunkStreamId & 0xff));
    chunkStreamId >>= 8;
    basicHeader->push_back(static_cast<uint8_t>(chunkStreamId & 0xff));
  } else if (chunkStreamId >= 64) {
    basicHeader->push_back(0);
    chunkStreamId -= 64;
    basicHeader->push_back(static_cast<uint8_t>(chunkStreamId & 0xff));
  } else {
    basicHeader->push_back(static_cast<uint8_t>(chunkStreamId & 0x3f));
  }

  (*basicHeader)[0] |= static_cast<uint8_t>(chunk_format) & 0xc0;

  return basicHeader;
}

//====================================================================================================
// MakeChunkRawHeader
//====================================================================================================
std::shared_ptr<std::vector<uint8_t>> RtmpMuxUtil::MakeChunkRawHeader(const std::shared_ptr<Rtmp::ChunkHeader> &header,
                                                                      bool isEextend) {
  if (!header)
    return nullptr;

  auto basicHeader = MakeChunkBasicHeader(header->basicHeader.formatType, header->basicHeader.chunkStreamId);
  auto rawHeader = std::make_shared<std::vector<uint8_t>>(basicHeader->begin(), basicHeader->end());

  switch (header->basicHeader.formatType) {
  case Rtmp::ChunkFormat::Type_0:
    if (!isEextend)
      WriteInt24(rawHeader, header->type_0.timestamp);
    else
      WriteInt24(rawHeader, Rtmp::ExtendFormat);
    WriteInt24(rawHeader, header->type_0.bodySize);
    rawHeader->push_back(header->type_0.typeId);
    WriteInt32LE(rawHeader, header->type_0.streamId);
    if (isEextend)
      WriteInt32(rawHeader, header->type_0.timestamp);
    break;
  case Rtmp::ChunkFormat::Type_1:
    if (!isEextend)
      WriteInt24(rawHeader, header->type_1.timestampDelta);
    else
      WriteInt24(rawHeader, Rtmp::ExtendFormat);
    WriteInt24(rawHeader, header->type_1.bodySize);
    rawHeader->push_back(header->type_1.typeId);
    if (isEextend)
      WriteInt32(rawHeader, header->type_1.timestampDelta);
    break;
  case Rtmp::ChunkFormat::Type_2:
    if (!isEextend)
      WriteInt24(rawHeader, header->type_2.timestampDelta);
    else {
      WriteInt24(rawHeader, Rtmp::ExtendFormat);
      WriteInt32(rawHeader, header->type_2.timestampDelta);
    }
    break;
  default:
    break;
  }

  return rawHeader;
}

//====================================================================================================
// GetChunkDataRawSize
//====================================================================================================
int RtmpMuxUtil::GetChunkDataRawSize(int chunkSize, uint32_t chunkStreamId, int chunkDataSize, bool isEextend) {
  if (chunkSize == 0 || chunkDataSize == 0)
    return 0;

  int type3HeaderSize = GetBasicHeaderSizeByChunkStreamID(chunkStreamId);
  if (isEextend)
    type3HeaderSize += Rtmp::ExtendTimestampSize;

  int blockCount = (chunkDataSize - 1) / chunkSize;
  return chunkDataSize + blockCount * type3HeaderSize;
}

//====================================================================================================
// MakeChunkRawData
//====================================================================================================
std::shared_ptr<std::vector<uint8_t>>
RtmpMuxUtil::MakeChunkRawData(int chunkSize, uint32_t chunkStreamId,
                              const std::shared_ptr<std::vector<uint8_t>> &chunkData, bool isEextend, uint32_t time) {
  if (chunkSize == 0 || chunkData->empty())
    return nullptr;

  int readSize = 0;
  int blockCount = static_cast<int>((chunkData->size() - 1) / chunkSize) + 1;
  auto basicHeader = MakeChunkBasicHeader(Rtmp::ChunkFormat::Type_3, chunkStreamId);
  auto chunkRawData = std::make_shared<std::vector<uint8_t>>();

  for (int index = 0; index < blockCount; ++index) {
    if (index > 0) {
      chunkRawData->insert(chunkRawData->end(), basicHeader->begin(), basicHeader->end());
      if (isEextend)
        WriteInt32(chunkRawData, time);
    }

    int size = std::min(static_cast<int>(chunkData->size()) - readSize, chunkSize);
    chunkRawData->insert(chunkRawData->end(), chunkData->data() + readSize, chunkData->data() + readSize + size);
    readSize += size;
  }

  return chunkRawData;
}