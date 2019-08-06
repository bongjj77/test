#pragma once
#include "./common/common_function.h"
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
	StreamManager(MainObject * pMainObject);
	virtual ~StreamManager();
	
public :


private :	
	MainObject									*_main_object;
}; 

