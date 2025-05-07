#include "rtmp_media_parser.h"
#include <iomanip>
#include <iostream>
#include <unordered_map>

namespace Rtmp {

std::shared_ptr<H264Config> MediaParser::H264SeqParse(const std::shared_ptr<std::vector<uint8_t>> &avcSeqHeader) {
  Bitop bitop(avcSeqHeader);

  // Skip first 40 bits
  bitop.Skip(40);

  auto info = std::make_shared<H264Config>();

  // Save avcConfiguration data
  info->avcConf = std::make_shared<std::vector<uint8_t>>(avcSeqHeader->begin() + 5, avcSeqHeader->end());

  // Read configuration version
  info->confVersion = bitop.Read(8);

  // Read profile, compatibility, level
  info->profile = bitop.Read(8);
  info->compatibility = bitop.Read(8);
  info->level = bitop.Read(8);

  bitop.Skip(6);
  bitop.Read(2); // lengthSizeMinusOne

  uint8_t numOfSPS = bitop.Read(8) & 0x1F;

  if (numOfSPS > 0) {
    uint16_t spsSize = bitop.Read(16);

    if (spsSize > 0) {
      auto spsData = std::make_shared<std::vector<uint8_t>>(spsSize);
      for (uint16_t i = 0; i < spsSize; ++i) {
        (*spsData)[i] = bitop.Read(8);
      }
      ParseSPS(info, spsData);
    }
  }

  uint8_t numOfPPS = bitop.Read(8);

  if (numOfPPS > 0) {
    uint16_t ppsSize = bitop.Read(16);

    if (ppsSize > 0) {
      bitop.Skip(ppsSize * 8);
    }
  }

  // Logging avcConf data
  std::cout << "=== Video Config ===" << std::endl;
  std::cout << "Codec ID: " << static_cast<int>(info->codecId) << std::endl;
  std::cout << "Configuration Version: " << static_cast<int>(info->confVersion) << std::endl;
  std::cout << "Profile: " << static_cast<int>(info->profile) << std::endl;
  std::cout << "Compatibility: " << static_cast<int>(info->compatibility) << std::endl;
  std::cout << "Level: " << info->level << std::endl;
  std::cout << "Width: " << info->width << std::endl;
  std::cout << "Height: " << info->height << std::endl;
  std::cout << "AVC Configuration: ";
  // for (const auto &byte : *info->avcConf) {
  //   std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
  // }

  return info;
}

uint32_t MediaParser::ReadGolomb(Bitop &bitop) {
  uint32_t zeros = 0;
  while (bitop.Read(1) == 0) {
    ++zeros;
  }
  uint32_t result = (1 << zeros) - 1 + bitop.Read(zeros);
  return result;
}

void MediaParser::ParseSPS(const std::shared_ptr<H264Config> &info,
                           const std::shared_ptr<std::vector<uint8_t>> &spsData) {
  Bitop bitop(spsData);

  bitop.Skip(8); // Skip NAL header

  uint32_t chromaFormatIdc = 1;
  uint32_t separateColourPlaneFlag = 0;

  uint32_t profile_idc = bitop.Read(8);
  uint32_t constraint_set_flags = bitop.Read(6);
  uint32_t reserved_zero_2bits = bitop.Read(2);
  uint32_t level_idc = bitop.Read(8);
  uint32_t seq_parameter_set_id = ReadGolomb(bitop);

  if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244 || profile_idc == 44 ||
      profile_idc == 83 || profile_idc == 86 || profile_idc == 118 || profile_idc == 128) {
    chromaFormatIdc = ReadGolomb(bitop);
    if (chromaFormatIdc == 3) {
      separateColourPlaneFlag = bitop.Read(1);
    }
    ReadGolomb(bitop);   // bit_depth_luma_minus8
    ReadGolomb(bitop);   // bit_depth_chroma_minus8
    bitop.Skip(1);       // qpprime_y_zero_transform_bypass_flag
    if (bitop.Read(1)) { // seq_scaling_matrix_present_flag
      for (int i = 0; i < ((chromaFormatIdc != 3) ? 8 : 12); ++i) {
        if (bitop.Read(1)) { // seq_scaling_list_present_flag
                             // Skip scaling list
        }
      }
    }
  }

  ReadGolomb(bitop); // log2_max_frame_num_minus4
  uint32_t picOrderCntType = ReadGolomb(bitop);
  if (picOrderCntType == 0) {
    ReadGolomb(bitop); // log2_max_pic_order_cnt_lsb_minus4
  } else if (picOrderCntType == 1) {
    bitop.Skip(1);     // delta_pic_order_always_zero_flag
    ReadGolomb(bitop); // offset_for_non_ref_pic
    ReadGolomb(bitop); // offset_for_top_to_bottom_field
    uint32_t numRefFramesInPicOrderCntCycle = ReadGolomb(bitop);
    for (uint32_t i = 0; i < numRefFramesInPicOrderCntCycle; ++i) {
      ReadGolomb(bitop); // offset_for_ref_frame
    }
  }

  ReadGolomb(bitop); // max_num_ref_frames
  bitop.Skip(1);     // gaps_in_frame_num_value_allowed_flag

  uint32_t picWidthInMbsMinus1 = ReadGolomb(bitop);
  uint32_t picHeightInMapUnitsMinus1 = ReadGolomb(bitop);

  uint32_t frameMbsOnlyFlag = bitop.Read(1);
  if (!frameMbsOnlyFlag) {
    bitop.Skip(1); // mb_adaptive_frame_field_flag
  }

  bitop.Skip(1); // direct_8x8_inference_flag

  uint32_t frameCropLeftOffset = 0;
  uint32_t frameCropRightOffset = 0;
  uint32_t frameCropTopOffset = 0;
  uint32_t frameCropBottomOffset = 0;

  if (bitop.Read(1)) { // frame_cropping_flag
    frameCropLeftOffset = ReadGolomb(bitop);
    frameCropRightOffset = ReadGolomb(bitop);
    frameCropTopOffset = ReadGolomb(bitop);
    frameCropBottomOffset = ReadGolomb(bitop);
  }

  uint32_t width = ((picWidthInMbsMinus1 + 1) * 16) - (frameCropLeftOffset * 2) - (frameCropRightOffset * 2);
  uint32_t height = ((2 - frameMbsOnlyFlag) * (picHeightInMapUnitsMinus1 + 1) * 16) - (frameCropTopOffset * 2) -
                    (frameCropBottomOffset * 2);

  info->width = width;
  info->height = height;
}

std::shared_ptr<AacConfig> MediaParser::AacSeqParse(const std::shared_ptr<std::vector<uint8_t>> &payload) {
  auto info = std::make_shared<AacConfig>();

  info->soundFormat = ((*payload)[0] >> 4) & 0x0f;
  info->soundType = (*payload)[0] & 0x01;
  info->soundSize = ((*payload)[0] >> 1) & 0x01;
  info->soundRate = ((*payload)[0] >> 2) & 0x03;

  static const std::unordered_map<uint8_t, std::string> AudioCodecName = {
      {0, "PCM"},         {1, "ADPCM"}, {2, "MP3"},    {7, "G.711 mu-law"},
      {8, "G.711 A-law"}, {10, "AAC"},  {11, "Speex"}, {14, "MP3 8 kHz"}};
  static const std::unordered_map<uint8_t, uint32_t> AudioSoundRate = {{0, 5512}, {1, 11025}, {2, 22050}, {3, 44100}};

  info->codecName = AudioCodecName.at(info->soundFormat);
  info->sampleRate = AudioSoundRate.at(info->soundRate);
  info->channels = info->soundType + 1;

  if (info->soundFormat == 4) {
    info->sampleRate = 16000; // Nellymoser 16 kHz
  } else if (info->soundFormat == 5 || info->soundFormat == 7 || info->soundFormat == 8) {
    info->sampleRate = 8000; // Nellymoser 8 kHz | G.711 A-law | G.711 mu-law
  } else if (info->soundFormat == 11) {
    info->sampleRate = 16000; // Speex
  } else if (info->soundFormat == 14) {
    info->sampleRate = 8000; // MP3 8 kHz
  }

  if ((info->soundFormat == 10 || info->soundFormat == 13) && (*payload)[1] == 0) {
    info->aacSequenceHeader = std::make_shared<std::vector<uint8_t>>(payload->begin(), payload->end());

    if (info->soundFormat == 10) {
      auto aacInfo = ReadAACSpecificConfig(info->aacSequenceHeader);
      info->audioProfileName = GetAACProfileName(aacInfo);
      info->sampleRate = aacInfo->sampleRate;
      info->sampleIndex = aacInfo->sampleIndex;
      info->channels = aacInfo->channels;
    } else {
      info->sampleRate = 48000;
      info->channels = (*payload)[11];
    }
  }

  info->sampleSize = (info->soundSize + 1) * 8;

  // Logging aacSequenceHeader data
  std::cout << "=== Audio Config ===" << std::endl;
  std::cout << "Codec Name: " << info->codecName << std::endl;
  std::cout << "Sample Rate: " << info->sampleRate << std::endl;
  std::cout << "Channels: " << static_cast<int>(info->channels) << std::endl;
  std::cout << "Sample Size: " << info->sampleSize << std::endl;
  // if (info->aacSequenceHeader) {
  //   std::cout << "AAC Sequence Header: ";
  //   for (const auto &byte : *info->aacSequenceHeader) {
  //     std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
  //   }
  //   std::cout << std::dec << std::endl;
  // }

  return info;
}

std::shared_ptr<AacConfig>
MediaParser::ReadAACSpecificConfig(const std::shared_ptr<std::vector<uint8_t>> &aacSequenceHeader) {
  auto info = std::make_shared<AacConfig>();
  Bitop bitop(aacSequenceHeader);

  bitop.Read(16); // Skip first 16 bits
  info->objectType = GetObjectType(bitop);
  auto result = GetSampleRate(bitop, *info);
  info->sampleRate = result.first;
  info->sampleIndex = result.second;
  info->chanConfig = bitop.Read(4);

  static const std::vector<uint8_t> AacChannels = {0, 1, 2, 3, 4, 5, 6, 8};
  if (info->chanConfig < AacChannels.size()) {
    info->channels = AacChannels[info->chanConfig];
  }

  info->sbr = -1;
  info->ps = -1;

  if (info->objectType == 5 || info->objectType == 29) {
    if (info->objectType == 29) {
      info->ps = 1;
    }
    info->extObjectType = 5;
    info->sbr = 1;
    auto result = GetSampleRate(bitop, *info);
    info->sampleRate = result.first;
    info->sampleIndex = result.second;
    info->objectType = GetObjectType(bitop);
  }

  return info;
}

std::string MediaParser::GetAACProfileName(const std::shared_ptr<AacConfig> &info) {
  switch (info->objectType) {
  case 1:
    return "Main";
  case 2:
    if (info->ps > 0) {
      return "HEv2";
    }
    if (info->sbr > 0) {
      return "HE";
    }
    return "LC";
  case 3:
    return "SSR";
  case 4:
    return "LTP";
  case 5:
    return "SBR";
  default:
    return "";
  }
}

uint32_t MediaParser::GetObjectType(Bitop &bitop) { return bitop.Read(5); }

std::pair<uint32_t, uint32_t> MediaParser::GetSampleRate(Bitop &bitop, AacConfig &info) {
  static const std::vector<uint32_t> SampleRates = {96000, 88200, 64000, 48000, 44100, 32000, 24000,
                                                    22050, 16000, 12000, 11025, 8000,  7350};
  uint32_t sampleIndex = bitop.Read(4);
  uint32_t sampleRate;
  if (sampleIndex == 0x0f) {
    sampleRate = bitop.Read(24);
  } else {
    sampleRate = SampleRates[sampleIndex];
  }
  return std::make_pair(sampleRate, sampleIndex);
}

} // namespace Rtmp