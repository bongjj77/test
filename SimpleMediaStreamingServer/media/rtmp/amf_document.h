//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once

#include <string.h>
#include <vector>
#include <string>

enum class AmfDataType : int32_t 
{
    Null = 0,
    Undefined,
    Number,
    Boolean,
    String,
    Array,
    Object,
};

enum class AmfTypeMarker : int32_t 
{
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

class AmfObjectArray;
class AmfObject;
class AmfArray;
class AmfProperty;
class AmfDocument;

//====================================================================================================
// AmfUtil
//====================================================================================================
class AmfUtil 
{
public:
    AmfUtil() = default;

    virtual ~AmfUtil() = default;

public:
    static int WriteInt8(void *data, uint8_t number);
	static int WriteInt16(void *data, uint16_t number);
	static int WriteInt24(void *data, uint32_t number);
	static int WriteInt32(void *data, uint32_t number);

    static uint8_t ReadInt8(void *data);
	static uint16_t ReadInt16(void *data);
	static uint32_t ReadInt24(void *data);
	static uint32_t ReadInt32(void *data);

public:
     
    static int EncodeNumber(void *data, double number);
	static int EncodeBoolean(void *data, bool boolean);
	static int EncodeString(void *data, char *string);

    static int DecodeNumber(void *data, double *number);
	static int DecodeBoolean(void *data, bool *boolean);
	static int DecodeString(void *data, char *string);
};

//====================================================================================================
// AmfProperty
//====================================================================================================
class AmfProperty : public AmfUtil 
{
public:
    AmfProperty();

    explicit AmfProperty(AmfDataType type);
    explicit AmfProperty(double number);
    explicit AmfProperty(bool boolean);
    explicit AmfProperty(const char *string);    
    explicit AmfProperty(std::shared_ptr<AmfArray> array);            
    explicit AmfProperty(std::shared_ptr<AmfObject> object);    

	~AmfProperty() override;

public:
    void Dump(std::string &dump_string);

public:
    int Encode(void *data); // return == 0 fail, type -> packet
    int Decode(void *data, int data_length); // return == 0 fail, packet -> type

public:
    AmfDataType GetType() { return _amf_data_type; }
    double		GetNumber() { return _number; }
    bool		GetBoolean() { return _boolean; }
    char *		GetString() { return _string->data(); }
	std::shared_ptr<AmfArray> GetArray() { return _array; }
	std::shared_ptr<AmfObject> GetObject() { return _object; }

private:
    void Initialize();

private:
    AmfDataType _amf_data_type;
    double		_number;
    bool		_boolean;
	std::shared_ptr<std::vector<char>> _string;
	std::shared_ptr<AmfArray> _array;
	std::shared_ptr<AmfObject> _object;
};

//====================================================================================================
// AmfObjectArray
//====================================================================================================
class AmfObjectArray : AmfUtil 
{
public:
    explicit AmfObjectArray(AmfDataType type);

    ~AmfObjectArray() override;

public:

    struct PairProperty
	{
		PairProperty(const char * name)
		{
			_name = name;
			_property = nullptr;
		}

        std::string _name;
        std::shared_ptr<AmfProperty> _property;
    };  

public:
    virtual void Dump(std::string &dump_string);

public:
    int Encode(void *data); // return == 0 fail, type -> packet
    int Decode(void *data, int data_length); // return == 0 fail, packet -> type

public:
    bool AddProperty(const char *name, AmfDataType type);
    bool AddProperty(const char *name, double number);
    bool AddProperty(const char *name, bool boolean);
    bool AddProperty(const char *name, const char *string);
    bool AddProperty(const char *name, std::shared_ptr<AmfArray> array);
    bool AddProperty(const char *name, std::shared_ptr<AmfObject> object);
    bool AddNullProperty(const char *name);

public:
    int FindName(const char *name); // return  < 0 fail 
    const char * GetName(int index);
	AmfDataType GetType(int index);
	double GetNumber(int index);
	bool GetBoolean(int index);
	char * GetString(int index);
	std::shared_ptr<AmfArray> GetArray(int index);
	std::shared_ptr<AmfObject> GetObject(int index);

private:
	std::shared_ptr<PairProperty> GetPair(int index);

private:
    AmfDataType _amf_data_type;
    std::vector<std::shared_ptr<PairProperty>> _amf_property_pair_list;
};


//====================================================================================================
// AmfArray
//====================================================================================================
class AmfArray : public AmfObjectArray
{
public:
    AmfArray();
    ~AmfArray() override = default;

public:
    void Dump(std::string &dump_string) final;
};

//====================================================================================================
// AmfObject
//====================================================================================================
class AmfObject : public AmfObjectArray 
{
public:
    AmfObject();
    ~AmfObject() override = default;

public:
    void Dump(std::string &dump_string) final;
};

//====================================================================================================
// AmfDocument
//====================================================================================================
class AmfDocument : AmfUtil
{
public:
    AmfDocument();
    ~AmfDocument() override;

public:
    void Dump(std::string &dump_string);

public:
    int Encode(void *data);                    // return == 0 fail
    int Decode(void *data, int data_length);        // return == 0 fail

public:
    bool AddProperty(AmfDataType type);
    bool AddProperty(double number);
    bool AddProperty(bool boolean);
    bool AddProperty(const char * string);     
    bool AddProperty(std::shared_ptr<AmfArray> array);         
    bool AddProperty(std::shared_ptr<AmfObject> object);       

public:
    int GetPropertyCount();
    int GetPropertyIndex(char * name);        // return -1 fail
	std::shared_ptr<AmfProperty> GetProperty(int index);

private:

	std::vector<std::shared_ptr<AmfProperty>> _amf_property_list;

};
 

