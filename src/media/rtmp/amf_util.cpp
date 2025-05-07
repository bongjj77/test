#include "amf_util.h"
#include <algorithm>

//====================================================================================================
// AmfUtil - WriteInt8
//====================================================================================================
int AmfUtil::WriteInt8(uint8_t *data, uint8_t number) {
  if (data == nullptr) {
    return 0;
  }
  data[0] = number;
  return 1;
}

//====================================================================================================
// AmfUtil - WriteInt16
//====================================================================================================
int AmfUtil::WriteInt16(uint8_t *data, uint16_t number) {
  if (data == nullptr) {
    return 0;
  }
  data[0] = (number >> 8);
  data[1] = (number & 0xff);
  return 2;
}

//====================================================================================================
// AmfUtil - WriteInt24
//====================================================================================================
int AmfUtil::WriteInt24(uint8_t *data, uint32_t number) {
  if (data == nullptr) {
    return 0;
  }
  data[0] = (number >> 16);
  data[1] = (number >> 8);
  data[2] = (number & 0xff);
  return 3;
}

//====================================================================================================
// AmfUtil - WriteInt32
//====================================================================================================
int AmfUtil::WriteInt32(uint8_t *data, uint32_t number) {
  if (data == nullptr) {
    return 0;
  }
  data[0] = (number >> 24);
  data[1] = (number >> 16);
  data[2] = (number >> 8);
  data[3] = (number & 0xff);
  return 4;
}

//====================================================================================================
// AmfUtil - ReadInt8
//====================================================================================================
uint8_t AmfUtil::ReadInt8(const uint8_t *data) {
  if (data == nullptr) {
    return 0;
  }
  return data[0];
}

//====================================================================================================
// AmfUtil - ReadInt16
//====================================================================================================
uint16_t AmfUtil::ReadInt16(const uint8_t *data) {
  if (data == nullptr) {
    return 0;
  }
  return (data[0] << 8) | data[1];
}

//====================================================================================================
// AmfUtil - ReadInt24
//====================================================================================================
uint32_t AmfUtil::ReadInt24(const uint8_t *data) {
  if (data == nullptr) {
    return 0;
  }
  return (data[0] << 16) | (data[1] << 8) | data[2];
}

//====================================================================================================
// AmfUtil - ReadInt32
//====================================================================================================
uint32_t AmfUtil::ReadInt32(const uint8_t *data) {
  if (data == nullptr) {
    return 0;
  }
  return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

//====================================================================================================
// AmfUtil - EncodeNumber
//====================================================================================================
int AmfUtil::EncodeNumber(uint8_t *data, double number) {
  if (data == nullptr) {
    return 0;
  }
  data[0] = static_cast<uint8_t>(AmfTypeMarker::Number);
  std::memcpy(data + 1, &number, sizeof(number));
  std::reverse(data + 1, data + 1 + sizeof(number));
  return 1 + sizeof(number);
}

//====================================================================================================
// AmfUtil - EncodeBoolean
//====================================================================================================
int AmfUtil::EncodeBoolean(uint8_t *data, bool boolean) {
  if (data == nullptr) {
    return 0;
  }
  data[0] = static_cast<uint8_t>(AmfTypeMarker::Boolean);
  data[1] = boolean ? 1 : 0;
  return 2;
}

//====================================================================================================
// AmfUtil - EncodeString
//====================================================================================================
int AmfUtil::EncodeString(uint8_t *data, const char *string) {
  if (data == nullptr || string == nullptr) {
    return 0;
  }
  std::size_t length = std::strlen(string);
  data[0] = static_cast<uint8_t>(AmfTypeMarker::String);
  AmfUtil::WriteInt16(data + 1, static_cast<uint16_t>(length));
  std::memcpy(data + 3, string, length);
  return 1 + 2 + length;
}

//====================================================================================================
// AmfUtil - DecodeNumber
//====================================================================================================
double AmfUtil::DecodeNumber(const uint8_t *data) {
  double result;
  uint8_t temp[sizeof(result)];
  std::memcpy(temp, data, sizeof(result));
  std::reverse(temp, temp + sizeof(result));
  std::memcpy(&result, temp, sizeof(result));
  return result;
}