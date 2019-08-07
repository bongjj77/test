//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once

//====================================================================================================
// BitWriter
//====================================================================================================
class BitWriter
{
public :
    BitWriter(uint32_t data_size);
    ~BitWriter();
	
public : 	
	void 			Write(uint32_t bit_count, uint32_t value);
    uint32_t 		GetBitCount(){ return _bit_count; }
    const uint8_t*	GetData(){ return _data; }
	uint32_t		GetDataSize(){ return _data_size; } 
	
private :
    uint8_t		*_data;
   	uint32_t   	_data_size;
    uint32_t  	_bit_count;
};

