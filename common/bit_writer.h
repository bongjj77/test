//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once

#include <vector>

//====================================================================================================
// BitWriter
//====================================================================================================
class BitWriter
{
public :
    BitWriter(uint32_t data_size);
    ~BitWriter();
	
public : 	
	void  Write(uint32_t bit_count, uint32_t value);
    uint32_t GetBitCount(){ return _bit_count; }
	std::shared_ptr<std::vector<uint8_t>> GetData() { return _data;}	 
  	
private :
	std::shared_ptr<std::vector<uint8_t>> _data; 
	uint32_t _bit_count;
};

