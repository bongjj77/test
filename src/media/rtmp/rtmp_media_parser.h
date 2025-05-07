#pragma once
#include "amf_document.h"
#include "common/common_header.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace Rtmp {

struct MetaInfo {
  float videoFps = 0;
  double videoBps = 0;
  uint32_t videoWidth = 0;
  uint32_t videoHeight = 0;
  double audioBps = 0;
  std::string encoder;
};

struct H264Config {
  uint8_t profile;
  uint8_t compatibility;
  float level;
  uint8_t confVersion;
  uint32_t width;
  uint32_t height;
  uint8_t codecId;
  std::shared_ptr<std::vector<uint8_t>> avcConf;
};

struct AacConfig {
  uint8_t soundFormat;
  uint8_t soundType;
  uint8_t soundSize;
  uint8_t soundRate;
  std::string codecName;
  uint32_t sampleRate;
  uint8_t channels;
  std::shared_ptr<std::vector<uint8_t>> aacSequenceHeader;
  std::string audioProfileName;
  uint8_t objectType;
  uint8_t chanConfig;
  int8_t sbr;
  int8_t ps;
  uint8_t extObjectType;
  uint32_t sampleIndex;
  uint32_t sampleSize;
};

struct MediaInfo {
  std::shared_ptr<H264Config> video = nullptr;
  std::shared_ptr<AacConfig> audio = nullptr;
  std::shared_ptr<std::vector<uint8_t>> videoSeqHeader;
  std::shared_ptr<std::vector<uint8_t>> audioSeqHeader;
  std::shared_ptr<AmfDoc> metaData;
  std::shared_ptr<MetaInfo> metaInfo = nullptr;

  std::string ToString() {
    auto videoBps = metaInfo ? metaInfo->videoBps : 0;
    auto videoFps = metaInfo ? metaInfo->videoFps : 0;
    auto encoder = metaInfo ? metaInfo->encoder : "";

    return StringHelper::Format("video(%d/%.2fk/%.2ffps/%d*%d) audio(%s/%dch/%dhz/%d/%d) encoder(%s)",
                                video ? video->codecId : 0, videoBps, videoFps, video ? video->width : 0,
                                video ? video->height : 0, audio ? audio->codecName.c_str() : "",
                                audio ? audio->channels : 0, audio ? audio->sampleRate : 0,
                                audio ? audio->sampleIndex : 0, audio ? audio->sampleSize : 0, encoder.c_str());
  }
};

class Bitop {
public:
  explicit Bitop(const std::shared_ptr<std::vector<uint8_t>> &data, size_t bitOffset = 0)
      : data_(data), bitOffset_(bitOffset) {}

  uint32_t Read(uint32_t bitCount) {
    if (bitCount > 32 || bitOffset_ + bitCount > data_->size() * 8) {
      throw std::out_of_range("Attempt to read beyond buffer size");
    }

    uint32_t value = 0;
    for (uint32_t i = 0; i < bitCount; ++i) {
      value <<= 1;
      value |= ((*data_)[bitOffset_ / 8] >> (7 - (bitOffset_ % 8))) & 1;
      ++bitOffset_;
    }
    return value;
  }

  void Skip(uint32_t bitCount) {
    if (bitOffset_ + bitCount > data_->size() * 8) {
      throw std::out_of_range("Attempt to skip beyond buffer size");
    }
    bitOffset_ += bitCount;
  }

  size_t GetSizeInBits() const { return data_->size() * 8; }
  size_t GetBitOffset() const { return bitOffset_; }

private:
  std::shared_ptr<std::vector<uint8_t>> data_;
  size_t bitOffset_;
};

class MediaParser {
public:
  std::shared_ptr<H264Config> H264SeqParse(const std::shared_ptr<std::vector<uint8_t>> &avcSeqHeader);
  std::shared_ptr<AacConfig> AacSeqParse(const std::shared_ptr<std::vector<uint8_t>> &payload);

private:
  void ParseSPS(const std::shared_ptr<H264Config> &info, const std::shared_ptr<std::vector<uint8_t>> &spsData);
  std::shared_ptr<AacConfig> ReadAACSpecificConfig(const std::shared_ptr<std::vector<uint8_t>> &aacSequenceHeader);
  std::string GetAACProfileName(const std::shared_ptr<AacConfig> &info);

  uint32_t GetObjectType(Bitop &bitop);
  std::pair<uint32_t, uint32_t> GetSampleRate(Bitop &bitop, AacConfig &info);
  uint32_t ReadGolomb(Bitop &bitop);
};

} // namespace Rtmp
