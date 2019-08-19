//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "m4s_writer.h"

//====================================================================================================
// Constructor
//====================================================================================================
M4sWriter::M4sWriter(M4sMediaType media_type)
{
	_media_type = media_type;
}

//====================================================================================================
// Data Write(std::shared_ptr<std::vector<uint8_t>>)
//====================================================================================================
bool M4sWriter::WriteData(const std::shared_ptr<std::vector<uint8_t>> &data, std::shared_ptr<std::vector<uint8_t>> &data_stream)
{
	data_stream->insert(data_stream->end(), data->begin(), data->end());
	return true;
}

//====================================================================================================
// Data Write(std::vector<uint8_t>)
//====================================================================================================
bool M4sWriter::WriteData(const std::vector<uint8_t> &data, std::shared_ptr<std::vector<uint8_t>>& data_stream)
{
	data_stream->insert(data_stream->end(), data.begin(), data.end());
	return true;
}

//====================================================================================================
// Data Write
//====================================================================================================
bool M4sWriter::WriteData(const uint8_t *data, int data_size, std::shared_ptr<std::vector<uint8_t>> &data_stream)
{
	data_stream->insert(data_stream->end(), data, data + data_size);
	return true;
}

//====================================================================================================
// write init
//====================================================================================================
bool M4sWriter::WriteInit(uint8_t value, int init_size, std::shared_ptr<std::vector<uint8_t>> &data_stream)
{
	for (int index = 0; index < init_size; index++)
	{
		data_stream->push_back(value);
	}

	return true;
}

//====================================================================================================
// Text write
//====================================================================================================
bool M4sWriter::WriteText(std::string value, std::shared_ptr<std::vector<uint8_t>> &data_stream)
{
	data_stream->insert(data_stream->end(), value.begin(), value.end());
	return true;
}

//====================================================================================================
// 64bit write
//====================================================================================================
bool M4sWriter::WriteUint64(uint64_t value, std::shared_ptr<std::vector<uint8_t>> &data_stream)
{
	data_stream->push_back((uint8_t)(value >> 56 & 0xFF));
	data_stream->push_back((uint8_t)(value >> 48 & 0xFF));
	data_stream->push_back((uint8_t)(value >> 40 & 0xFF));
	data_stream->push_back((uint8_t)(value >> 32 & 0xFF));
	data_stream->push_back((uint8_t)(value >> 24 & 0xFF));
	data_stream->push_back((uint8_t)(value >> 16 & 0xFF));
	data_stream->push_back((uint8_t)(value >> 8 & 0xFF));
	data_stream->push_back((uint8_t)(value & 0xFF));

	return true;
}

//====================================================================================================
// 32bit write
//====================================================================================================
bool M4sWriter::WriteUint32(uint32_t value, std::shared_ptr<std::vector<uint8_t>> &data_stream)
{
	data_stream->push_back((uint8_t)(value >> 24 & 0xFF));
	data_stream->push_back((uint8_t)(value >> 16 & 0xFF));
	data_stream->push_back((uint8_t)(value >> 8 & 0xFF));
	data_stream->push_back((uint8_t)(value & 0xFF));

	return true;
}

//====================================================================================================
// 24bit write
//====================================================================================================
bool M4sWriter::WriteUint24(uint32_t value, std::shared_ptr<std::vector<uint8_t>> &data_stream)
{
	data_stream->push_back((uint8_t)(value >> 16 & 0xFF));
	data_stream->push_back((uint8_t)(value >> 8 & 0xFF));
	data_stream->push_back((uint8_t)(value & 0xFF));
	return true;
}

//====================================================================================================
// 16bit write 
//====================================================================================================
bool M4sWriter::WriteUint16(uint16_t value, std::shared_ptr<std::vector<uint8_t>> &data_stream)
{
	data_stream->push_back((uint8_t)(value >> 8 & 0xFF));
	data_stream->push_back((uint8_t)(value & 0xFF));
	return true;
}

//====================================================================================================
// 8bit write
//====================================================================================================
bool M4sWriter::WriteUint8(uint8_t value, std::shared_ptr<std::vector<uint8_t>> &data_stream)
{
	data_stream->push_back(value);
	return true;
}

//====================================================================================================
// Box data
//====================================================================================================
int M4sWriter::BoxDataWrite(std::string type,
							const std::shared_ptr<std::vector<uint8_t>> &data,
							std::shared_ptr<std::vector<uint8_t>> &data_stream)
{
	uint32_t box_size = (data != nullptr) ? M4S_BOX_HEADER_SIZE + data->size() : M4S_BOX_HEADER_SIZE;

	WriteUint32(box_size, data_stream); // box size write
	WriteText(type, data_stream);	    // type write

	if(data != nullptr)
	{
		data_stream->insert(data_stream->end(), data->begin(), data->end());// data write
	}

	return data_stream->size();
}

//====================================================================================================
// Box data
//====================================================================================================
int M4sWriter::BoxDataWrite(std::string type,
							uint8_t version,
							uint32_t flags,
							const std::shared_ptr<std::vector<uint8_t>> &data,
							std::shared_ptr<std::vector<uint8_t>> &data_stream)
{
	uint32_t box_size = (data != nullptr) ? M4S_BOX_EXT_HEADER_SIZE + data->size() : M4S_BOX_EXT_HEADER_SIZE;

	WriteUint32(box_size, data_stream);    // box size write
	WriteText(type, data_stream);          // type write
	WriteUint8(version, data_stream);	   // version write
	WriteUint24(flags, data_stream);       // flag write

	if(data != nullptr)
	{
		data_stream->insert(data_stream->end(), data->begin(), data->end());// data write
	}

	return data_stream->size();
}

//====================================================================================================
// Box data
//====================================================================================================
int M4sWriter::BoxDataWrite(const std::string type,
							const std::shared_ptr<std::vector<uint8_t>>& data,
							std::shared_ptr<std::vector<uint8_t>>& data_stream,
							bool data_size_write/* = false*/)
{
	uint32_t box_size = (data != nullptr) ? M4S_BOX_HEADER_SIZE + data->size() : M4S_BOX_HEADER_SIZE;

	// supported mdat box(data copy decrease)
	if (data_size_write && data != nullptr)
		box_size += sizeof(uint32_t);


	WriteUint32(box_size, data_stream); 	// box size write
	WriteText(type, data_stream);			// type write

	if (data != nullptr)
	{
		if (data_size_write)
			WriteUint32(data->size(), data_stream);

		data_stream->insert(data_stream->end(), data->begin(), data->end());// data write
	}

	return data_stream->size();
}