#pragma once
#include "amf_util.h"
#include <memory>
#include <string>
#include <vector>

class AmfObjectArray;
class AmfObject;
class AmfArray;
class AmfProperty;
class AmfDoc;

//====================================================================================================
// AmfProperty
//====================================================================================================
class AmfProperty : public AmfUtil {
public:
  AmfProperty() = default;
  explicit AmfProperty(AmfDataType type);
  explicit AmfProperty(double number);
  explicit AmfProperty(bool boolean);
  explicit AmfProperty(const char *string);
  explicit AmfProperty(std::shared_ptr<AmfArray> array);
  explicit AmfProperty(std::shared_ptr<AmfObject> object);

  ~AmfProperty() override = default;

  std::string Dump();
  int Encode(uint8_t *data);               // return == 0 fail, type -> packet
  int Decode(uint8_t *data, int dataSize); // return == 0 fail, packet -> type
  AmfDataType GetType() const { return _dataType; }
  double GetNumber() const { return _number; }
  bool GetBoolean() const { return _boolean; }
  const char *GetString() const { return _string ? _string->data() : nullptr; }
  std::shared_ptr<AmfArray> GetArray() const { return _array; }
  std::shared_ptr<AmfObject> GetObject() const { return _object; }

private:
  AmfDataType _dataType = AmfDataType::Null;
  double _number = 0.0;
  bool _boolean = true;
  std::shared_ptr<std::vector<char>> _string;
  std::shared_ptr<AmfArray> _array;
  std::shared_ptr<AmfObject> _object;
};

//====================================================================================================
// AmfObjectArray
//====================================================================================================
class AmfObjectArray : public AmfUtil {
public:
  explicit AmfObjectArray(AmfDataType type);

  ~AmfObjectArray() override = default;

  struct PairProperty {
    explicit PairProperty(const std::string &name) : _name(name) {}

    std::string _name;
    std::shared_ptr<AmfProperty> _property;
  };

  virtual std::string Dump();

  int Encode(uint8_t *data);               // return == 0 fail, type -> packet
  int Decode(uint8_t *data, int dataSize); // return == 0 fail, packet -> type

  bool AddProp(const char *name, AmfDataType type);
  bool AddProp(const char *name, double number);
  bool AddProp(const char *name, bool boolean);
  bool AddProp(const char *name, const char *string);
  bool AddProp(const char *name, std::shared_ptr<AmfArray> array);
  bool AddProp(const char *name, std::shared_ptr<AmfObject> object);
  bool AddNullProperty(const char *name);

  int FindName(const char *name) const; // return  < 0 fail
  const char *GetName(int index) const;
  AmfDataType GetType(int index) const;
  double GetNumber(int index) const;
  bool GetBoolean(int index) const;
  const char *GetString(int index) const;
  std::shared_ptr<AmfArray> GetArray(int index) const;
  std::shared_ptr<AmfObject> GetObject(int index) const;

private:
  std::shared_ptr<PairProperty> GetPair(int index) const;

private:
  AmfDataType _dataType;
  std::vector<std::shared_ptr<PairProperty>> _propertyPairList;
};

//====================================================================================================
// AmfArray
//====================================================================================================
class AmfArray : public AmfObjectArray {
public:
  AmfArray();
  ~AmfArray() override = default;

  std::string Dump() final;
};

//====================================================================================================
// AmfObject
//====================================================================================================
class AmfObject : public AmfObjectArray {
public:
  AmfObject();
  ~AmfObject() override = default;

  std::string Dump() final;
};

//====================================================================================================
// AmfDoc
//====================================================================================================
class AmfDoc : public AmfUtil {
public:
  AmfDoc();
  ~AmfDoc() override;

  std::string Dump();
  int Encode(uint8_t *data) const;                               // return == 0 fail
  int Decode(const std::shared_ptr<std::vector<uint8_t>> &data); // return == 0 fail
  bool AddProp(AmfDataType type);
  bool AddProp(double number);
  bool AddProp(bool boolean);
  bool AddProp(const char *string);
  bool AddProp(const std::string &string);
  bool AddProp(std::shared_ptr<AmfArray> array);
  bool AddProp(std::shared_ptr<AmfObject> object);
  int GetPropertyCount() const;
  int GetPropertyIndex(const char *name) const; // return -1 fail
  std::shared_ptr<AmfProperty> GetProp(int index) const;

private:
  std::vector<std::shared_ptr<AmfProperty>> _properties;
};
