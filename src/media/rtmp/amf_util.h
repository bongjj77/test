#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

enum class AmfDataType : int32_t {
  Null = 0,
  Undefined,
  Number,
  Boolean,
  String,
  Array,
  Object,
};

enum class AmfTypeMarker : uint8_t {
  Number = 0,
  Boolean,
  String,
  Object,
  MovieClip,
  Null,
  Undefined,
  Reference,
  EcmaArray,
  ObjectEnd,
  StrictArray,
  Date,
  LongString,
  Unsupported,
  Recordset,
  Xml,
  TypedObject,
};

//====================================================================================================
// AmfUtil
//====================================================================================================
class AmfUtil {
public:
  AmfUtil() = default;
  virtual ~AmfUtil() = default;

  static int WriteInt8(uint8_t *data, uint8_t number);
  static int WriteInt16(uint8_t *data, uint16_t number);
  static int WriteInt24(uint8_t *data, uint32_t number);
  static int WriteInt32(uint8_t *data, uint32_t number);

  static uint8_t ReadInt8(const uint8_t *data);
  static uint16_t ReadInt16(const uint8_t *data);
  static uint32_t ReadInt24(const uint8_t *data);
  static uint32_t ReadInt32(const uint8_t *data);

  static int EncodeNumber(uint8_t *data, double number);
  static int EncodeBoolean(uint8_t *data, bool boolean);
  static int EncodeString(uint8_t *data, const char *string);

  static double DecodeNumber(const uint8_t *data);
};