//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "amf_document.h"

//====================================================================================================
// AmfUtil - WriteInt8
//====================================================================================================
int AmfUtil::WriteInt8(void *data, uint8_t number)
{
    auto *output = (uint8_t *) data;
 
    if (!data) 
	{ 
		return 0; 
	}
 
    output[0] = number;

    return 1;
}

//====================================================================================================
// AmfUtil - WriteInt16
//====================================================================================================
int AmfUtil::WriteInt16(void *data, uint16_t number)
{
    auto *output = (uint8_t *) data;
 
    if (!data) 
	{ 
		return 0; 
	}
 
    output[0] = (uint8_t) (number >> 8);
    output[1] = (uint8_t) (number & 0xff);

    return 2;
}

//====================================================================================================
// AmfUtil - WriteInt24
//====================================================================================================
int AmfUtil::WriteInt24(void *data, uint32_t number)
{
    auto *output = (uint8_t *) data;
	 
    if (!data) 
	{ 
		return 0; 
	}
 
    output[0] = (uint8_t) (number >> 16);
    output[1] = (uint8_t) (number >> 8);
    output[2] = (uint8_t) (number & 0xff);

    return 3;
}

//====================================================================================================
// AmfUtil - WriteInt32
//====================================================================================================
int AmfUtil::WriteInt32(void *data, uint32_t number)
{
    auto *output = (uint8_t *) data;
	 
    if (!data) 
	{ 
		return 0; 
	}
 
    output[0] = (uint8_t) (number >> 24);
    output[1] = (uint8_t) (number >> 16);
    output[2] = (uint8_t) (number >> 8);
    output[3] = (uint8_t) (number & 0xff);

    return 4;
}

//====================================================================================================
// AmfUtil - ReadInt8
//====================================================================================================
uint8_t AmfUtil::ReadInt8(void *data)
{
    auto *pt_in = (uint8_t *) data;
    uint8_t number = 0;
 
    if (!data) 
	{ 
		return 0; 
	}
	 
    number |= pt_in[0];

    return number;
}

//====================================================================================================
// AmfUtil - ReadInt16
//====================================================================================================
uint16_t AmfUtil::ReadInt16(void *data)
{
    auto *pt_in = (uint8_t *) data;
    uint16_t number = 0;
 
    if (!data) 
	{ 
		return 0; 
	}
 
    number |= pt_in[0] << 8;
    number |= pt_in[1];

    return number;
}

//====================================================================================================
// AmfUtil - ReadInt24
//====================================================================================================
uint32_t AmfUtil::ReadInt24(void *data)
{
    auto *pt_in = (uint8_t *) data;
    uint32_t number = 0;
 
    if (!data) 
	{ 
		return 0; 
	}
 
    number |= pt_in[0] << 16;
    number |= pt_in[1] << 8;
    number |= pt_in[2];

    return number;
}

//====================================================================================================
// AmfUtil - ReadInt32
//====================================================================================================
uint32_t AmfUtil::ReadInt32(void *data)
{
    auto *pt_in = (uint8_t *) data;
    uint32_t number = 0;
 
    if (!data) { return 0; }
 
    number |= pt_in[0] << 24;
    number |= pt_in[1] << 16;
    number |= pt_in[2] << 8;
    number |= pt_in[3];

    return number;
}

//====================================================================================================
// AmfUtil - WriteInt8
//====================================================================================================
int AmfUtil::EncodeNumber(void *data, double number)
{
    auto *pt_in = (uint8_t *) &number;
    auto *output = (uint8_t *) data;
	 
    if (!data) { return 0; }
 
    output += WriteInt8(output, (int) AmfTypeMarker::Number);
	 
    output[0] = pt_in[7];
    output[1] = pt_in[6];
    output[2] = pt_in[5];
    output[3] = pt_in[4];
    output[4] = pt_in[3];
    output[5] = pt_in[2];
    output[6] = pt_in[1];
    output[7] = pt_in[0];

    return (1 + 8);
}

//====================================================================================================
// AmfUtil - WriteInt8
//====================================================================================================
int AmfUtil::EncodeBoolean(void *data, bool boolean)
{
    auto *output = (uint8_t *) data;
 
    if (!data) { return 0; }
 
    output += WriteInt8(output, (int) AmfTypeMarker::Boolean);
 
    output[0] = (boolean ? 1 : 0);

    return (1 + 1);
}

//====================================================================================================
// AmfUtil - EncodeString
//====================================================================================================
int AmfUtil::EncodeString(void *data, char *string)
{
    auto *output = (uint8_t *) data;
 
    if (!data || !string) { return 0; }
 
    output += WriteInt8(output, (int) AmfTypeMarker::String);
 
    output += WriteInt16(output, (uint16_t) strlen(string));
    strncpy((char *) output, string, strlen(string));

    return (1 + 2 + (int) strlen(string));
}

//====================================================================================================
// AmfUtil - DecodeNumber
//====================================================================================================
int AmfUtil::DecodeNumber(void *data, double *number)
{
    auto *pt_in = (uint8_t *) data;
    auto *output = (uint8_t *) number;
 
    if (!data || !number) { return 0; }
 
    if (ReadInt8(pt_in) != (int) AmfTypeMarker::Number) { return 0; }
    pt_in++;
 
    output[0] = pt_in[7];
    output[1] = pt_in[6];
    output[2] = pt_in[5];
    output[3] = pt_in[4];
    output[4] = pt_in[3];
    output[5] = pt_in[2];
    output[6] = pt_in[1];
    output[7] = pt_in[0];

    return (1 + 8);
}

//====================================================================================================
// AmfUtil - DecodeBoolean
//====================================================================================================
int AmfUtil::DecodeBoolean(void *data, bool *boolean)
{
    auto *pt_in = (uint8_t *) data;
	 
    if (!data || !boolean) { return 0; }
 
    if (ReadInt8(pt_in) != (int) AmfTypeMarker::Boolean) { return 0; }
    pt_in++;
	 
    *boolean = pt_in[0];

    return (1 + 1);
}

//====================================================================================================
// AmfUtil - DecodeString
//====================================================================================================
int AmfUtil::DecodeString(void *data, char *string)
{
    auto *pt_in = (uint8_t *) data;
    int str_len;
	 
    if (!data || !string) { return 0; }
	 
    if (ReadInt8(pt_in) != (int) AmfTypeMarker::String) { return 0; }
    pt_in++;
	 
    str_len = (int) ReadInt16(pt_in);
    pt_in += 2;
    //
    strncpy(string, (char *) pt_in, str_len);
    string[str_len] = '\0';

    return (1 + 2 + str_len);
}
 
//====================================================================================================
// AmfProperty - AmfProperty
//====================================================================================================
AmfProperty::AmfProperty()
{
	Initialize();
}

//====================================================================================================
// AmfProperty - AmfProperty
//====================================================================================================
AmfProperty::AmfProperty(AmfDataType type)
{
    Initialize();
 
    _amf_data_type = type;
}

//====================================================================================================
// AmfProperty - AmfProperty
//====================================================================================================
AmfProperty::AmfProperty(double number)
{ 
    Initialize();
 
    _amf_data_type = AmfDataType::Number;
    _number = number;
}

//====================================================================================================
// AmfProperty - AmfProperty
//====================================================================================================
AmfProperty::AmfProperty(bool boolean)
{ 
    Initialize();
 
    _amf_data_type = AmfDataType::Boolean;
    _boolean = boolean;
}

//====================================================================================================
// AmfProperty - AmfProperty
//====================================================================================================
AmfProperty::AmfProperty(const char *string) // 스트링은 내부에서 메모리 할당해서 복사
{
    Initialize();

    if (!string) { return; }

    _string = new char[strlen(string) + 1];
    strcpy(_string, string);
    _amf_data_type = AmfDataType::String;
}

//====================================================================================================
// AmfProperty - AmfProperty
//====================================================================================================
AmfProperty::AmfProperty(AmfArray *pArray) // array 는 파라미터 포인터를 그대로 저장
{
    Initialize();

    if (!pArray) { return; }

    _amf_data_type = AmfDataType::Array;
    _array = pArray;
}

//====================================================================================================
// AmfProperty - AmfProperty
//====================================================================================================
AmfProperty::AmfProperty(AmfObject *pObject) // object 는 파라미터 포인터를 그대로 저장
{
    Initialize();

    if (!pObject) { return; }

    _amf_data_type = AmfDataType::Object;
    _object = pObject;
}

//====================================================================================================
// AmfProperty - ~AmfProperty
//====================================================================================================
AmfProperty::~AmfProperty()
{
    if (_string)
    {
        delete [] _string;
        _string = nullptr;
    }

    if (_array)
    {
        delete _array;
        _array = nullptr;
    }

    if (_object)
    {
        delete _object;
        _object = nullptr;
    }
}

//====================================================================================================
// AmfProperty - Initialize
//====================================================================================================
void AmfProperty::Initialize()
{
    _amf_data_type = AmfDataType::Null;
    _number = 0.0;
    _boolean = true;
    _string = nullptr;
    _array = nullptr;
    _object = nullptr;
}

//====================================================================================================
// AmfProperty - Dump
//====================================================================================================
void AmfProperty::Dump(std::string &dump_string)
{
    char text[1024] = {0,};
    switch (GetType())
    {
        case AmfDataType::Null:
            dump_string += ("nullptr\n");
            break;
        case AmfDataType::Undefined:
            dump_string += ("Undefined\n");
            break;
        case AmfDataType::Number:
            sprintf(text, "%.1f\n", GetNumber());
            dump_string += text;
            break;
        case AmfDataType::Boolean:
            sprintf(text, "%d\n", GetBoolean());
            dump_string += text;
            break;
        case AmfDataType::String:
            if (_string)
            {
                sprintf(text, "%s\n", strlen(GetString()) ? GetString() : "SIZE-ZERO STRING");
                dump_string += text;
            }
            break;
        case AmfDataType::Array:
            if (_array)
            {
                _array->Dump(dump_string);
            }
            break;
        case AmfDataType::Object:
            if (_object)
            {
                _object->Dump(dump_string);
            }
            break;
    }
}

//====================================================================================================
// AmfProperty - Encode
// - return 0 fail
//====================================================================================================
int AmfProperty::Encode(void *data)  
{
    auto *output = (uint8_t *) data;

	if (!data) 
	{ 
		return 0; 
	}
 
    switch (_amf_data_type)
    {
        case AmfDataType::Null:
            output += WriteInt8(output, (int) AmfTypeMarker::Null);
            break;
        case AmfDataType::Undefined:
            output += WriteInt8(output, (int) AmfTypeMarker::Undefined);
            break;
        case AmfDataType::Number:
            output += EncodeNumber(output, _number);
            break;
        case AmfDataType::Boolean:
            output += EncodeBoolean(output, _boolean);
            break;
        case AmfDataType::String:
            if (!_string) { break; }
            output += EncodeString(output, _string);
            break;
        case AmfDataType::Array:
            if (!_array) { break; }
            output += _array->Encode(output);
            break;
        case AmfDataType::Object:
            if (!_object) { break; }
            output += _object->Encode(output);
            break;
        default:
            break;
    }

    return (int) (output - (uint8_t *) data);
}

//====================================================================================================
// AmfProperty - Decode
// retrun 0 fail
//====================================================================================================
int AmfProperty::Decode(void *data, int data_length)  
{
    auto *pt_in = (uint8_t *) data;
    int size = 0;

	if (!data) 
	{ 
		return 0; 
	}
    
	if (data_length < 1) 
	{ 
		return 0; 
	}

	_amf_data_type = AmfDataType::Null;

    switch ((AmfTypeMarker) ReadInt8(data))
    {
        case AmfTypeMarker::Null:
            size = 1;
            _amf_data_type = AmfDataType::Null;
            break;
        case AmfTypeMarker::Undefined:
            size = 1;
            _amf_data_type = AmfDataType::Undefined;
            break;
        case AmfTypeMarker::Number:
            if (data_length < (1 + 8)) { break; }
            size = DecodeNumber(pt_in, &_number);
            _amf_data_type = AmfDataType::Number;
            break;
        case AmfTypeMarker::Boolean:
            if (data_length < (1 + 1)) { return 0; }
            size = DecodeBoolean(pt_in, &_boolean);
            _amf_data_type = AmfDataType::Boolean;
            break;
        case AmfTypeMarker::String:
            if (data_length < (1 + 3)) { return 0; }
            if (_string)
            {
                delete [] _string;
                _string = nullptr;
            }
            _string = new char[ReadInt16(pt_in + 1) + 1];
            size = DecodeString(pt_in, _string);
            if (!size)
            {
                delete [] _string;
                _string = nullptr;
            }
            _amf_data_type = AmfDataType::String;
            break;
        case AmfTypeMarker::EcmaArray:
            if (_array)
            {
                delete _array;
                _array = nullptr;
            }
            _array = new AmfArray;
            size = _array->Decode(pt_in, data_length);
            if (!size)
            {
                delete _array;
                _array = nullptr;
            }
            _amf_data_type = AmfDataType::Array;
            break;
        case AmfTypeMarker::Object:
            if (_object)
            {
                delete _object;
                _object = nullptr;
            }
            _object = new AmfObject;
            size = _object->Decode(pt_in, data_length);
            if (!size)
            {
                delete _object;
                _object = nullptr;
            }
            _amf_data_type = AmfDataType::Object;
            break;
        default:
            break;
    }

    return size;
}

//====================================================================================================
// AmfObjectArray - AmfObjectArray
//====================================================================================================
AmfObjectArray::AmfObjectArray(AmfDataType Type)
{
    _amf_data_type = Type;
    _amf_property_pairs.clear();
}

//====================================================================================================
// AmfObjectArray - ~AmfObjectArray
//====================================================================================================
AmfObjectArray::~AmfObjectArray()
{
    for (int index = 0; index < (int) _amf_property_pairs.size(); index++)
    {
        if (_amf_property_pairs[index])
        {
            if (_amf_property_pairs[index]->_property)
            {
                delete _amf_property_pairs[index]->_property;
                _amf_property_pairs[index]->_property = nullptr;
            }
            delete _amf_property_pairs[index];
            _amf_property_pairs[index] = nullptr;
        }
    }
    _amf_property_pairs.clear();
}

//====================================================================================================
// AmfObjectArray - Dump
//====================================================================================================
void AmfObjectArray::Dump(std::string &dump_string)
{
    char text[1024] = {0,};

    for (int index = 0; index < (int) _amf_property_pairs.size(); index++)
    {
        sprintf(text, "%s : ", _amf_property_pairs[index]->_name);
        dump_string += text;

        if (_amf_property_pairs[index]->_property->GetType() == AmfDataType::Array ||
            _amf_property_pairs[index]->_property->GetType() == AmfDataType::Object)
        {
            dump_string += ("\n");
        }
        _amf_property_pairs[index]->_property->Dump(dump_string);
    }
}

//====================================================================================================
// AmfObjectArray - Encode
//====================================================================================================
int AmfObjectArray::Encode(void *data)
{
    auto *output = (uint8_t *) data;
    uint8_t start_marker;
    uint8_t end_marker;
    
    start_marker = (uint8_t) (_amf_data_type == AmfDataType::Object ? AmfTypeMarker::Object : AmfTypeMarker::EcmaArray);
    end_marker = (uint8_t) AmfTypeMarker::ObjectEnd;

    if (_amf_property_pairs.empty())
    {
        return 0;
    }

    output += WriteInt8(output, start_marker);

    if (_amf_data_type == AmfDataType::Array)
    {
        output += WriteInt32(output, 0); /* 0=infinite */
    }

	for (int index = 0; index < (int) _amf_property_pairs.size(); index++)
    {
        PairProperty *property_pair = nullptr;

        property_pair = _amf_property_pairs[index];
        if (!property_pair) { continue; }

        // property 가 invalid check
        //if( property_pair->_property->GetType() == AmfDataType::Null ) { continue; }

        output += WriteInt16(output, (uint16_t) strlen(property_pair->_name));
        memcpy((void *)output, (const void *)property_pair->_name, strlen(property_pair->_name));
        output += strlen(property_pair->_name);
 
        output += property_pair->_property->Encode(output);
    }
 
    output += WriteInt16(output, 0);
    output += WriteInt8(output, end_marker);

    return (int) (output - (uint8_t *) data);
}

//====================================================================================================
// AmfObjectArray - Decode
//====================================================================================================
int AmfObjectArray::Decode(void *data, int data_length)
{
    auto *pt_in = (uint8_t *) data;
    uint8_t start_marker;
    uint8_t end_marker;
 
    start_marker = (uint8_t) (_amf_data_type == AmfDataType::Object ? AmfTypeMarker::Object : AmfTypeMarker::EcmaArray);
    end_marker = (uint8_t) AmfTypeMarker::ObjectEnd;
 
    if (ReadInt8(pt_in) != start_marker) { return 0; }
    pt_in++;
 
    if (_amf_data_type == AmfDataType::Array)
    {
        pt_in += sizeof(uint32_t);
    }
 
    while (true)
    {
        PairProperty *property_pair = nullptr;
        int len;
 
        if (ReadInt8(pt_in) == end_marker)
        {
            pt_in++;
            break;
        }
 
        len = (int) ReadInt16(pt_in);
        pt_in += sizeof(uint16_t);
        if (len == 0)
        {
            continue;
        }
 
        property_pair = new PairProperty;
        property_pair->_property = new AmfProperty;
 
        strncpy(property_pair->_name, (char *) pt_in, len);
        pt_in += len;
        property_pair->_name[len] = '\0';
 
        len = property_pair->_property->Decode(pt_in, data_length - (int) (pt_in - (uint8_t *) data));
        pt_in += len;

        // value check
        //if( !len || property_pair->_property->GetType() == AmfDataType::Null )
        if (len == 0) {
            delete property_pair->_property;
            property_pair->_property = nullptr;

            delete property_pair;
            property_pair = nullptr;
            return 0;
        }
 
        _amf_property_pairs.push_back(property_pair);
        property_pair = nullptr;
    }

    return (int) (pt_in - (uint8_t *) data);
}

//====================================================================================================
// AmfObjectArray - AddProperty
//====================================================================================================
bool AmfObjectArray::AddProperty(const char *name, AmfDataType type)
{
    PairProperty *property_pair = nullptr;
 
    if (!name) 
	{ 
		return false; 
	}

    if (type != AmfDataType::Null && type != AmfDataType::Undefined) 
	{ 
		return false; 
	}
 
    property_pair = new PairProperty;
    strcpy(property_pair->_name, name);
    property_pair->_property = new AmfProperty(type);
 
    _amf_property_pairs.push_back(property_pair);
    property_pair = nullptr;

    return true;
}

//====================================================================================================
// AmfObjectArray - AddProperty
//====================================================================================================
bool AmfObjectArray::AddProperty(const char *name, double number)
{
    PairProperty *property_pair = nullptr;
	 
    if (!name) 
	{ 
		return false; 
	}
 
    property_pair = new PairProperty;
    strcpy(property_pair->_name, name);
    property_pair->_property = new AmfProperty(number);
 
    _amf_property_pairs.push_back(property_pair);
    property_pair = nullptr;

    return true;
}

//====================================================================================================
// AmfObjectArray - AddProperty
//====================================================================================================
bool AmfObjectArray::AddProperty(const char *name, bool boolean)
{
    PairProperty *property_pair = nullptr;
 
    if (!name) 
	{ 
		return false; 
	}
 
    property_pair = new PairProperty;
    strcpy(property_pair->_name, name);
    property_pair->_property = new AmfProperty(boolean);
 
    _amf_property_pairs.push_back(property_pair);
    property_pair = nullptr;

    return true;
}

//====================================================================================================
// AmfObjectArray - AddProperty
//====================================================================================================
bool AmfObjectArray::AddProperty(const char *name, const char *string)
{
    PairProperty *property_pair = nullptr;
 
    if (!name) 
	{ 
		return false; 
	}
 
    property_pair = new PairProperty;
    strcpy(property_pair->_name, name);
    property_pair->_property = new AmfProperty(string);
	 
    _amf_property_pairs.push_back(property_pair);
    property_pair = nullptr;

    return true;
}

//====================================================================================================
// AmfObjectArray - AddProperty
//====================================================================================================
bool AmfObjectArray::AddProperty(const char *name, AmfArray *array)
{
    PairProperty *property_pair = nullptr;
 
    if (!name) 
	{ 
		return false; 
	}
 
    property_pair = new PairProperty;
    strcpy(property_pair->_name, name);
    property_pair->_property = new AmfProperty(array);
 
    _amf_property_pairs.push_back(property_pair);
    property_pair = nullptr;

    return true;
}

//====================================================================================================
// AmfObjectArray - AddProperty
//====================================================================================================
bool AmfObjectArray::AddProperty(const char *name, AmfObject *object)
{
    PairProperty *property_pair = nullptr;
	 
    if (!name) 
	{ 
		return false; 
	}

	property_pair = new PairProperty;
    strcpy(property_pair->_name, name);
    property_pair->_property = new AmfProperty(object);
 
    _amf_property_pairs.push_back(property_pair);
    property_pair = nullptr;

    return true;
}

//====================================================================================================
// AmfObjectArray - AddNullProperty
//====================================================================================================
bool AmfObjectArray::AddNullProperty(const char *name)
{
    PairProperty *property_pair = nullptr;
 
    if (!name) 
	{ 
		return false; 
	}
 
    property_pair = new PairProperty;
    strcpy(property_pair->_name, name);
    property_pair->_property = new AmfProperty();

    _amf_property_pairs.push_back(property_pair);
    property_pair = nullptr;

    return true;
}

//====================================================================================================
// AmfObjectArray - GetPair
//====================================================================================================
AmfObject::PairProperty *AmfObjectArray::GetPair(int index)
{ 
    if (index < 0) 
	{ 
		return nullptr; 
	}

    if (index >= (int) _amf_property_pairs.size()) 
	{ 
		return nullptr; 
	}

    return _amf_property_pairs[index];
}

//====================================================================================================
// AmfObjectArray - FindName
// return < 0 fail
//====================================================================================================
int AmfObjectArray::FindName(const char *name) 
{
    if (!name) 
	{ 
		return -1; 
	}
	 
    for (int index = 0; index < (int) _amf_property_pairs.size(); index++)
    {
        if (!strcmp(_amf_property_pairs[index]->_name, name))
        {
            return index;
        }
    }

    return -1;
}

//====================================================================================================
// AmfObjectArray - GetName
//====================================================================================================
char *AmfObjectArray::GetName(int index)
{
    PairProperty *property_pair = nullptr;
	 
    property_pair = GetPair(index);
    if (!property_pair) { return nullptr; }

    return property_pair->_name;
}

//====================================================================================================
// AmfObjectArray - GetType
//====================================================================================================
AmfDataType AmfObjectArray::GetType(int index)
{
    PairProperty *property_pair = nullptr;
	 
    property_pair = GetPair(index);
    if (!property_pair) { return AmfDataType::Null; }

    return property_pair->_property->GetType();
}

//====================================================================================================
// AmfObjectArray - GetNumber
//====================================================================================================
double AmfObjectArray::GetNumber(int index)
{
    PairProperty *property_pair = nullptr;
 
    property_pair = GetPair(index);
    if (!property_pair) { return 0; }

    return property_pair->_property->GetNumber();
}

//====================================================================================================
// AmfObjectArray - GetBoolean
//====================================================================================================
bool AmfObjectArray::GetBoolean(int index)
{
    PairProperty *property_pair = nullptr;
	 
    property_pair = GetPair(index);
    if (!property_pair) { return false; }

    return property_pair->_property->GetBoolean();
}

//====================================================================================================
// AmfObjectArray - GetString
//====================================================================================================
char *AmfObjectArray::GetString(int index)
{
    PairProperty *property_pair = nullptr;
	 
    property_pair = GetPair(index);

    if (!property_pair) 
	{ 
		return nullptr; 
	}

    return property_pair->_property->GetString();
}

//====================================================================================================
// AmfObjectArray - GetArray
//====================================================================================================
AmfArray *AmfObjectArray::GetArray(int index)
{
    PairProperty *property_pair = nullptr;
 
    property_pair = GetPair(index);
    if (!property_pair) { return nullptr; }

    return property_pair->_property->GetArray();
}

//====================================================================================================
// AmfObjectArray - GetObject
//====================================================================================================
AmfObject *AmfObjectArray::GetObject(int index)
{
    PairProperty *property_pair = nullptr;
 
    property_pair = GetPair(index);
    if (!property_pair) { return nullptr; }

    return property_pair->_property->GetObject();
}
 
//====================================================================================================
// AmfArray - AmfObjectArray
//====================================================================================================
AmfArray::AmfArray() : AmfObjectArray(AmfDataType::Array)
{

}

//====================================================================================================
// AmfArray - Dump
//====================================================================================================
void AmfArray::Dump(std::string &dump_string)
{
    dump_string += ("\n=> Array Start <=\n");
    AmfObjectArray::Dump(dump_string);
    dump_string += ("=> Array  End  <=\n");
}

//====================================================================================================
// AmfObject - AmfObject
//====================================================================================================
AmfObject::AmfObject() : AmfObjectArray(AmfDataType::Object)
{

}

//====================================================================================================
// AmfObject - Dump
//====================================================================================================
void AmfObject::Dump(std::string &dump_string)
{
    dump_string += ("=> Object Start <=\n");
    AmfObjectArray::Dump(dump_string);
    dump_string += ("=> Object  End  <=\n");
}

//====================================================================================================
// AmfDocument - AmfDocument
//====================================================================================================
AmfDocument::AmfDocument()
{
    _amf_propertys.clear();
}

//====================================================================================================
// AmfDocument - ~AmfDocument
//====================================================================================================
AmfDocument::~AmfDocument()
{
  
	for (int index = 0; index < (int) _amf_propertys.size(); index++)
    {
        if (_amf_propertys[index])
        {
            delete _amf_propertys[index];
            _amf_propertys[index] = nullptr;
        }
    }
    _amf_propertys.clear();
}

//====================================================================================================
// AmfDocument - Dump
//====================================================================================================
void AmfDocument::Dump(std::string &dump_string)
{
    dump_string += ("\n======= DUMP START =======\n");
    for (int index = 0; index < (int) _amf_propertys.size(); index++)
    {
        _amf_propertys[index]->Dump(dump_string);
    }
    dump_string += ("\n======= DUMP END =======\n");
}

//====================================================================================================
// AmfDocument - Encode
// return 0 fail
//====================================================================================================
int AmfDocument::Encode(void *data)  
{
    auto *output = (uint8_t *) data;
    
    for (int index = 0; index < (int) _amf_propertys.size(); index++)
    {
        output += _amf_propertys[index]->Encode(output);
    }

    return (int) (output - (uint8_t *) data);
}

//====================================================================================================
// AmfDocument - Decode
// return 0 fail
//====================================================================================================
int AmfDocument::Decode(void *data, int data_length)  
{
    auto *pt_in = (uint8_t *) data;
    int total_length = 0;
    int return_length;
 
    while (total_length < data_length)
    {
        AmfProperty *pt_item = nullptr;
 
        pt_item = new AmfProperty;
 
        return_length = pt_item->Decode(pt_in, data_length - total_length);
        if (!return_length)
        {
            delete pt_item;
            pt_item = nullptr;
            break;
        }
 
        pt_in += return_length;
        total_length += return_length;
 
        _amf_propertys.push_back(pt_item);
        pt_item = nullptr;
    }

    return (int) (pt_in - (uint8_t *) data);
}

//====================================================================================================
// AmfDocument - AddProperty
//====================================================================================================
bool AmfDocument::AddProperty(AmfDataType type)
{ 
    if (type != AmfDataType::Null && type != AmfDataType::Undefined) { return false; }
 
    _amf_propertys.push_back(new AmfProperty(type));

    return true;
}

//====================================================================================================
// AmfDocument - AddProperty
//====================================================================================================
bool AmfDocument::AddProperty(double number)
{
    _amf_propertys.push_back(new AmfProperty(number));

    return true;
}

//====================================================================================================
// AmfDocument - AddProperty
//====================================================================================================
bool AmfDocument::AddProperty(bool boolean)
{  
    _amf_propertys.push_back(new AmfProperty(boolean));

    return true;
}

//====================================================================================================
// AmfDocument - AddProperty
//====================================================================================================
bool AmfDocument::AddProperty(const char *string)
{  
    if (!string) 
	{ 
		return false; 
	}
 
    _amf_propertys.push_back(new AmfProperty(string));

    return true;
}

//====================================================================================================
// AmfDocument - AddProperty
//====================================================================================================
bool AmfDocument::AddProperty(AmfArray *array)
{
    if (!array) 
	{ 
		return false; 
	}

    _amf_propertys.push_back(new AmfProperty(array));

    return true;
}

//====================================================================================================
// AmfDocument - AddProperty
//====================================================================================================
bool AmfDocument::AddProperty(AmfObject *object)
{
    if (!object) 
	{ 
		return false; 
	}

    _amf_propertys.push_back(new AmfProperty(object));

    return true;
}

//====================================================================================================
// AmfDocument - GetPropertyCount
//====================================================================================================
int AmfDocument::GetPropertyCount()
{
    return (int) _amf_propertys.size();
}

//====================================================================================================
// AmfDocument - GetPropertyIndex
//====================================================================================================
int AmfDocument::GetPropertyIndex(char *name)
{
     
    if (!name)
    {
        return -1;
    }

    if (_amf_propertys.empty())
    {
        return -1;
    }

	for (int index = 0; index < (int) _amf_propertys.size(); index++)
    {
        if (_amf_propertys[index]->GetType() != AmfDataType::String) { continue; }
        if (strcmp(_amf_propertys[index]->GetString(), name)) { continue; }

        return index;
    }

    return -1;
}

//====================================================================================================
// AmfDocument - GetProperty
//====================================================================================================
AmfProperty * AmfDocument::GetProperty(int index)
{
	if (index < 0) 
	{ 
		return nullptr; 
	}

    if (index >= (int) _amf_propertys.size()) 
	{ 
		return nullptr; 
	}

    return _amf_propertys[index];
}