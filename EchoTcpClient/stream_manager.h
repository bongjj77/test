//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "common/common_function.h"
#include <mutex>
#include <memory>
#pragma pack(1)

class MainObject; 

#pragma pack()

//====================================================================================================
// StreamManager
//====================================================================================================
class StreamManager 
{
public:
	StreamManager(std::shared_ptr<MainObject> &main_object);
	virtual ~StreamManager();
	
public :


private :	
	std::shared_ptr<MainObject> _main_object;
}; 

