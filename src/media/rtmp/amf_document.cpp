#include "amf_document.h"
#include "common/string_helper.hpp"

//====================================================================================================
// AmfProperty - AmfProperty
//====================================================================================================
AmfProperty::AmfProperty(AmfDataType type) : _dataType(type) {}

AmfProperty::AmfProperty(double number) : _dataType(AmfDataType::Number), _number(number) {}

AmfProperty::AmfProperty(bool boolean) : _dataType(AmfDataType::Boolean), _boolean(boolean) {}

AmfProperty::AmfProperty(const char *data) {
  if (data) {
    _string = std::make_shared<std::vector<char>>(data, data + std::strlen(data) + 1);
    _dataType = AmfDataType::String;
  }
}

AmfProperty::AmfProperty(std::shared_ptr<AmfArray> array) : _dataType(AmfDataType::Array), _array(std::move(array)) {}

AmfProperty::AmfProperty(std::shared_ptr<AmfObject> object)
    : _dataType(AmfDataType::Object), _object(std::move(object)) {}

//====================================================================================================
// AmfProperty - Dump
//====================================================================================================
std::string AmfProperty::Dump() {
  switch (GetType()) {
  case AmfDataType::Null:
    return "nullptr\n";
  case AmfDataType::Undefined:
    return "Undefined\n";
  case AmfDataType::Number:
    return StringHelper::Format("%.1f\n", GetNumber());
  case AmfDataType::Boolean:
    return StringHelper::Format("%d\n", GetBoolean());
  case AmfDataType::String:
    return _string ? StringHelper::Format("%s\n", _string->data()) : "";
  case AmfDataType::Array:
    return _array ? _array->Dump() : "";
  case AmfDataType::Object:
    return _object ? _object->Dump() : "";
  default:
    return "";
  }
}

//====================================================================================================
// AmfProperty - Encode
// - return 0 fail
//====================================================================================================
int AmfProperty::Encode(uint8_t *data) {
  if (!data)
    return 0;

  auto *output = data;
  switch (_dataType) {
  case AmfDataType::Null:
    output += WriteInt8(output, static_cast<int>(AmfTypeMarker::Null));
    break;
  case AmfDataType::Undefined:
    output += WriteInt8(output, static_cast<int>(AmfTypeMarker::Undefined));
    break;
  case AmfDataType::Number:
    output += EncodeNumber(output, _number);
    break;
  case AmfDataType::Boolean:
    output += EncodeBoolean(output, _boolean);
    break;
  case AmfDataType::String:
    if (_string)
      output += EncodeString(output, _string->data());
    break;
  case AmfDataType::Array:
    if (_array)
      output += _array->Encode(output);
    break;
  case AmfDataType::Object:
    if (_object)
      output += _object->Encode(output);
    break;
  default:
    break;
  }
  return output - data;
}

//====================================================================================================
// AmfProperty - Decode
// return 0 fail
//====================================================================================================
int AmfProperty::Decode(uint8_t *data, int dataSize) {
  if (!data || dataSize < 1)
    return 0;

  int size = 0;
  auto typeMarker = static_cast<AmfTypeMarker>(ReadInt8(data));

  switch (typeMarker) {
  case AmfTypeMarker::Null:
    size = 1;
    _dataType = AmfDataType::Null;
    break;
  case AmfTypeMarker::Undefined:
    size = 1;
    _dataType = AmfDataType::Undefined;
    break;
  case AmfTypeMarker::Number:
    if (dataSize >= 9) {
      _number = DecodeNumber(data + 1);
      size = 9;
      _dataType = AmfDataType::Number;
    }
    break;
  case AmfTypeMarker::Boolean:
    if (dataSize >= 2) {
      _boolean = data[1];
      size = 2;
      _dataType = AmfDataType::Boolean;
    }
    break;
  case AmfTypeMarker::String: {
    if (dataSize >= 3) {
      int stringSize = ReadInt16(data + 1);
      if (dataSize >= stringSize + 3) {
        _string = std::make_shared<std::vector<char>>(data + 3, data + 3 + stringSize);
        _string->push_back(0);
        size = 3 + stringSize;
        _dataType = AmfDataType::String;
      }
    }
    break;
  }
  case AmfTypeMarker::EcmaArray: {
    _array = std::make_shared<AmfArray>();
    size = _array->Decode(data, dataSize);
    if (!size)
      _array = nullptr;
    _dataType = AmfDataType::Array;
    break;
  }
  case AmfTypeMarker::Object: {
    _object = std::make_shared<AmfObject>();
    size = _object->Decode(data, dataSize);
    if (!size)
      _object = nullptr;
    _dataType = AmfDataType::Object;
    break;
  }
  default:
    break;
  }
  return size;
}

//====================================================================================================
// AmfObjectArray - AmfObjectArray
//====================================================================================================
AmfObjectArray::AmfObjectArray(AmfDataType type) : _dataType(type) {}

//====================================================================================================
// AmfObjectArray - Dump
//====================================================================================================
std::string AmfObjectArray::Dump() {
  std::string dump;
  for (const auto &pair : _propertyPairList) {
    dump += StringHelper::Format("%s : ", pair->_name.c_str());
    dump += (pair->_property->GetType() == AmfDataType::Array || pair->_property->GetType() == AmfDataType::Object)
                ? "\n"
                : "";
    dump += pair->_property->Dump();
  }
  return dump;
}

//====================================================================================================
// AmfObjectArray - Encode
//====================================================================================================
int AmfObjectArray::Encode(uint8_t *data) {
  if (_propertyPairList.empty())
    return 0;

  auto *output = data;
  uint8_t startMarker =
      static_cast<uint8_t>(_dataType == AmfDataType::Object ? AmfTypeMarker::Object : AmfTypeMarker::EcmaArray);
  uint8_t endMarker = static_cast<uint8_t>(AmfTypeMarker::ObjectEnd);

  output += WriteInt8(output, startMarker);
  if (_dataType == AmfDataType::Array)
    output += WriteInt32(output, 0); // 0=infinite

  for (const auto &pair : _propertyPairList) {
    output += WriteInt16(output, static_cast<uint16_t>(pair->_name.size()));
    std::memcpy(output, pair->_name.data(), pair->_name.size());
    output += pair->_name.size();
    output += pair->_property->Encode(output);
  }

  output += WriteInt16(output, 0);
  output += WriteInt8(output, endMarker);
  return output - data;
}

//====================================================================================================
// AmfObjectArray - Decode
//====================================================================================================
int AmfObjectArray::Decode(uint8_t *data, int dataSize) {
  auto *dataPos = data;
  uint8_t startMarker =
      static_cast<uint8_t>(_dataType == AmfDataType::Object ? AmfTypeMarker::Object : AmfTypeMarker::EcmaArray);
  uint8_t endMarker = static_cast<uint8_t>(AmfTypeMarker::ObjectEnd);

  if (ReadInt8(dataPos) != startMarker)
    return 0;
  dataPos++;

  if (_dataType == AmfDataType::Array)
    dataPos += sizeof(uint32_t);

  while (dataPos <= data + dataSize) {
    if (ReadInt8(dataPos) == endMarker) {
      dataPos++;
      break;
    }

    int size = ReadInt16(dataPos);
    dataPos += sizeof(uint16_t);
    if (size == 0)
      continue;

    std::string name(dataPos, dataPos + size);
    dataPos += size;

    auto propertyPair = std::make_shared<PairProperty>(name);
    propertyPair->_property = std::make_shared<AmfProperty>();
    size = propertyPair->_property->Decode(dataPos, dataSize - static_cast<int>(dataPos - data));
    if (size == 0)
      return 0;

    dataPos += size;
    _propertyPairList.push_back(propertyPair);
  }
  return dataPos - data;
}

//====================================================================================================
// AmfObjectArray - AddProp
//====================================================================================================
bool AmfObjectArray::AddProp(const char *name, AmfDataType type) {
  if (!name || (type != AmfDataType::Null && type != AmfDataType::Undefined))
    return false;
  auto propertyPair = std::make_shared<PairProperty>(name);
  propertyPair->_property = std::make_shared<AmfProperty>(type);
  _propertyPairList.push_back(propertyPair);
  return true;
}

bool AmfObjectArray::AddProp(const char *name, double number) {
  if (!name)
    return false;
  auto propertyPair = std::make_shared<PairProperty>(name);
  propertyPair->_property = std::make_shared<AmfProperty>(number);
  _propertyPairList.push_back(propertyPair);
  return true;
}

bool AmfObjectArray::AddProp(const char *name, bool boolean) {
  if (!name)
    return false;
  auto propertyPair = std::make_shared<PairProperty>(name);
  propertyPair->_property = std::make_shared<AmfProperty>(boolean);
  _propertyPairList.push_back(propertyPair);
  return true;
}

bool AmfObjectArray::AddProp(const char *name, const char *string) {
  if (!name)
    return false;
  auto propertyPair = std::make_shared<PairProperty>(name);
  propertyPair->_property = std::make_shared<AmfProperty>(string);
  _propertyPairList.push_back(propertyPair);
  return true;
}

bool AmfObjectArray::AddProp(const char *name, std::shared_ptr<AmfArray> array) {
  if (!name)
    return false;
  auto propertyPair = std::make_shared<PairProperty>(name);
  propertyPair->_property = std::make_shared<AmfProperty>(array);
  _propertyPairList.push_back(propertyPair);
  return true;
}

bool AmfObjectArray::AddProp(const char *name, std::shared_ptr<AmfObject> object) {
  if (!name)
    return false;
  auto propertyPair = std::make_shared<PairProperty>(name);
  propertyPair->_property = std::make_shared<AmfProperty>(object);
  _propertyPairList.push_back(propertyPair);
  return true;
}

bool AmfObjectArray::AddNullProperty(const char *name) {
  if (!name)
    return false;
  auto propertyPair = std::make_shared<PairProperty>(name);
  propertyPair->_property = std::make_shared<AmfProperty>();
  _propertyPairList.push_back(propertyPair);
  return true;
}

//====================================================================================================
// AmfObjectArray - GetPair
//====================================================================================================
std::shared_ptr<AmfObjectArray::PairProperty> AmfObjectArray::GetPair(int index) const {
  if (index < 0 || index >= static_cast<int>(_propertyPairList.size()))
    return nullptr;
  return _propertyPairList[index];
}

//====================================================================================================
// AmfObjectArray - FindName
// return < 0 fail
//====================================================================================================
int AmfObjectArray::FindName(const char *name) const {
  if (!name)
    return -1;
  for (size_t index = 0; index < _propertyPairList.size(); index++) {
    if (_propertyPairList[index]->_name == name)
      return static_cast<int>(index);
  }
  return -1;
}

//====================================================================================================
// AmfObjectArray - GetName
//====================================================================================================
const char *AmfObjectArray::GetName(int index) const {
  auto propertyPair = GetPair(index);
  return propertyPair ? propertyPair->_name.c_str() : nullptr;
}

//====================================================================================================
// AmfObjectArray - GetType
//====================================================================================================
AmfDataType AmfObjectArray::GetType(int index) const {
  auto propertyPair = GetPair(index);
  return propertyPair ? propertyPair->_property->GetType() : AmfDataType::Null;
}

//====================================================================================================
// AmfObjectArray - GetNumber
//====================================================================================================
double AmfObjectArray::GetNumber(int index) const {
  auto propertyPair = GetPair(index);
  return propertyPair ? propertyPair->_property->GetNumber() : 0.0;
}

//====================================================================================================
// AmfObjectArray - GetBoolean
//====================================================================================================
bool AmfObjectArray::GetBoolean(int index) const {
  auto propertyPair = GetPair(index);
  return propertyPair ? propertyPair->_property->GetBoolean() : false;
}

//====================================================================================================
// AmfObjectArray - GetString
//====================================================================================================
const char *AmfObjectArray::GetString(int index) const {
  auto propertyPair = GetPair(index);
  return propertyPair ? propertyPair->_property->GetString() : nullptr;
}

//====================================================================================================
// AmfObjectArray - GetArray
//====================================================================================================
std::shared_ptr<AmfArray> AmfObjectArray::GetArray(int index) const {
  auto propertyPair = GetPair(index);
  return propertyPair ? propertyPair->_property->GetArray() : nullptr;
}

//====================================================================================================
// AmfObjectArray - GetObject
//====================================================================================================
std::shared_ptr<AmfObject> AmfObjectArray::GetObject(int index) const {
  auto propertyPair = GetPair(index);
  return propertyPair ? propertyPair->_property->GetObject() : nullptr;
}

//====================================================================================================
// AmfArray - AmfArray
//====================================================================================================
AmfArray::AmfArray() : AmfObjectArray(AmfDataType::Array) {}

//====================================================================================================
// AmfArray - Dump
//====================================================================================================
std::string AmfArray::Dump() {
  std::string dump = "\n=> Array Start <=\n";
  dump += AmfObjectArray::Dump();
  dump += "=> Array  End  <=\n";
  return dump;
}

//====================================================================================================
// AmfObject - AmfObject
//====================================================================================================
AmfObject::AmfObject() : AmfObjectArray(AmfDataType::Object) {}

//====================================================================================================
// AmfObject - Dump
//====================================================================================================
std::string AmfObject::Dump() {
  std::string dump = "=> Object Start <=\n";
  dump += AmfObjectArray::Dump();
  dump += "=> Object  End  <=\n";
  return dump;
}

//====================================================================================================
// AmfDoc - AmfDoc
//====================================================================================================
AmfDoc::AmfDoc() {}

//====================================================================================================
// AmfDoc - ~AmfDoc
//====================================================================================================
AmfDoc::~AmfDoc() {}

//====================================================================================================
// AmfDoc - Dump
//====================================================================================================
std::string AmfDoc::Dump() {
  std::string dump = "\n======= DUMP START =======\n";
  for (const auto &property : _properties) {
    dump += property->Dump();
  }
  dump += "======= DUMP END =======\n";
  return dump;
}

//====================================================================================================
// AmfDoc - Encode
// return 0 fail
//====================================================================================================
int AmfDoc::Encode(uint8_t *data) const {
  auto *output = data;
  for (const auto &property : _properties) {
    output += property->Encode(output);
  }
  return output - data;
}

//====================================================================================================
// AmfDoc - Decode
// return 0 fail
//====================================================================================================
int AmfDoc::Decode(const std::shared_ptr<std::vector<uint8_t>> &data) {
  int procSize = 0;
  while (procSize < static_cast<int>(data->size())) {
    auto item = std::make_shared<AmfProperty>();
    int size = item->Decode(data->data() + procSize, data->size() - procSize);
    if (size == 0)
      break;
    procSize += size;
    _properties.push_back(item);
  }
  return procSize;
}

//====================================================================================================
// AmfDoc - AddProp
//====================================================================================================
bool AmfDoc::AddProp(AmfDataType type) {
  if (type != AmfDataType::Null && type != AmfDataType::Undefined)
    return false;
  _properties.push_back(std::make_shared<AmfProperty>(type));
  return true;
}

bool AmfDoc::AddProp(double number) {
  _properties.push_back(std::make_shared<AmfProperty>(number));
  return true;
}

bool AmfDoc::AddProp(bool boolean) {
  _properties.push_back(std::make_shared<AmfProperty>(boolean));
  return true;
}

bool AmfDoc::AddProp(const char *string) {
  if (!string)
    return false;
  _properties.push_back(std::make_shared<AmfProperty>(string));
  return true;
}

bool AmfDoc::AddProp(const std::string &string) {
  if (string.empty())
    return false;
  _properties.push_back(std::make_shared<AmfProperty>(string.c_str()));
  return true;
}

bool AmfDoc::AddProp(std::shared_ptr<AmfArray> array) {
  if (!array)
    return false;
  _properties.push_back(std::make_shared<AmfProperty>(array));
  return true;
}

bool AmfDoc::AddProp(std::shared_ptr<AmfObject> object) {
  if (!object)
    return false;
  _properties.push_back(std::make_shared<AmfProperty>(object));
  return true;
}

//====================================================================================================
// AmfDoc - GetPropertyCount
//====================================================================================================
int AmfDoc::GetPropertyCount() const { return static_cast<int>(_properties.size()); }

//====================================================================================================
// AmfDoc - GetPropertyIndex
//====================================================================================================
int AmfDoc::GetPropertyIndex(const char *name) const {
  if (!name)
    return -1;
  for (size_t index = 0; index < _properties.size(); index++) {
    if (_properties[index]->GetType() != AmfDataType::String)
      continue;
    if (std::strcmp(_properties[index]->GetString(), name) == 0)
      return static_cast<int>(index);
  }
  return -1;
}

//====================================================================================================
// AmfDoc - GetProp
//====================================================================================================
std::shared_ptr<AmfProperty> AmfDoc::GetProp(int index) const {
  if (index < 0 || index >= static_cast<int>(_properties.size()))
    return nullptr;
  return _properties[index];
}