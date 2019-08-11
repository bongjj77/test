//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "test_tcp_client_manager.h" 


//====================================================================================================
// Constructor 
//====================================================================================================
TestTcpClintManager::TestTcpClintManager(int object_key) : TcpNetworkManager(object_key)
{
	 
}

//====================================================================================================
// Object 추가(Accepted) 
//====================================================================================================
bool TestTcpClintManager::AcceptedAdd(std::shared_ptr<NetTcpSocket> socket, 
									uint32_t ip, 
									int port, 
									ITestTcpClientCallback *callback, 
									int &index_key) 
{
	TcpNetworkObjectParam object_param; 
		
	//Object Param 설정 	
	object_param.object_key			= _object_key;
	object_param.socket 			= socket; 
	object_param.network_callback	= _network_callback; 
	object_param.object_callback	= callback; 
	object_param.object_name		= _object_name;

	//TcpNetworkObject 소멸자에서	삭제 처리 하므로 NULL로 설정 
	socket = nullptr;
	 
	auto object =  std::make_shared<TestTcpClientObject>();
	
	if(object->Create(&object_param) == false)
	{
		index_key = -1;
		return false;
	}
	 
	index_key = Insert(object);

	return index_key == -1 ? false : true;	
}
