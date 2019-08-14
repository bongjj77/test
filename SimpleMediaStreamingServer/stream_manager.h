//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "common/common_function.h"
#include "media/media_define.h"
#include <mutex>
#include <memory>
#pragma pack(1)

class MainObject; 

struct StreamInfo
{
	StreamKey stream_key;
	
	MediaInfo media_info;

	int rtmp_encoder_key; 




};

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
	
	std::map<StreamKey, StreamInfo> _stream_list;

}; 

