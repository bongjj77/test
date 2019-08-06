#include <iostream>
#include "bit_writer.h"

//====================================================================================================
// BitWriter 
//====================================================================================================
BitWriter::BitWriter(uint32_t data_size) : _data_size(data_size), _bit_count(0)
{
	if (data_size != 0)
	{
	   _data = new uint8_t[data_size];
	   memset(_data, 0, data_size);
	}
	else 
	{
	   _data = nullptr;
	}
}

//====================================================================================================
// ~BitWriter 
//====================================================================================================
BitWriter::~BitWriter() 
{ 
	if(_data != nullptr)
	{
		delete[] _data;
		_data = nullptr; 
	}
}

//====================================================================================================
// BitWriter
//====================================================================================================
void BitWriter::Write(uint32_t bit_count, uint32_t value)
{
	uint8_t * data 	= nullptr;
	uint32_t 	nSpace 	= 0; 
	
	data = _data;
	
    if (_bit_count + bit_count > _data_size*8) 
	{
		return;
    }

	data += _bit_count/8;

	nSpace = 8-(_bit_count%8);

	while (bit_count) 
	{
        uint32_t mask = bit_count==32 ? 0xFFFFFFFF : ((1<<bit_count)-1);
		
        if (bit_count <= nSpace) 
		{
            *data |= ((value&mask) << (nSpace-bit_count));
            _bit_count += bit_count;
            return;
        } 
		else 
        {
            *data |= ((value&mask) >> (bit_count-nSpace));
            ++data;
            _bit_count += nSpace;
            bit_count  -= nSpace;
            nSpace       = 8;
        }
    }
}