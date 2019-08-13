//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "http_client_manager.h" 


//====================================================================================================
// Constructor 
//====================================================================================================
HttpClientManager::HttpClientManager(int object_key) : TcpNetworkManager(object_key)
{
	 
}

//====================================================================================================
// Object 추가(Accepted) 
//====================================================================================================
bool HttpClientManager::AcceptedAdd(std::shared_ptr<NetTcpSocket> socket,
									uint32_t ip, 
									int port, 
									std::shared_ptr<IHttpClient> callback, 
									int &index_key) 
{
	TcpNetworkObjectParam object_param; 
		
	//Object Param 설정 	
	object_param.object_key			= _object_key;
	object_param.socket 			= socket; 
	object_param.network_callback	= _network_callback; 
	object_param.object_callback	= callback; 
	object_param.object_name		= _object_name;

	//TcpNetworkObject 소멸자에서	삭제 처리 하므로 nullptr로 설정 
	socket = nullptr;
	 
	auto object =  std::make_shared<HttpClientObject>();
	
	if(object->Create(&object_param) == false)
	{
		index_key = -1;
		return false;
	}
	 
	index_key = Insert(object);

	return index_key == -1 ? false : true;	
}
