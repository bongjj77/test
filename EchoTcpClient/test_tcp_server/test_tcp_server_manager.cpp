//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "test_tcp_server_manager.h" 

//====================================================================================================
// 생성자 
//====================================================================================================
TestTcpServerManager::TestTcpServerManager(int object_key) : TcpNetworkManager(object_key)
{
	 
}

//====================================================================================================
// Add object(Connected)
//====================================================================================================
bool TestTcpServerManager::ConnectedAdd(std::shared_ptr<NetTcpSocket> socket, 
										std::shared_ptr<ITestTcpServerCallback> callback, 
										int &index_key)
{
	 
	TcpNetworkObjectParam object_param;
	
	object_param.object_key			= _object_key;
	object_param.socket				= socket;
	object_param.network_callback	= _network_callback;
	object_param.object_callback	= callback;
	object_param.object_name		= _object_name;

	//TcpNetworkObject 소멸자에서	삭제 처리 하므로 NULL로 설정 
	socket = nullptr;

	auto object = std::make_shared<TestTcpServerObject>();

	if (!object->Create(&object_param))
	{
		index_key = -1;
		return false;
	}

	// Object 정보 추가 
	index_key = Insert(object, true, 20);

	return index_key == -1 ? false : true;
}


//====================================================================================================
// Echo packet send
//====================================================================================================
bool TestTcpServerManager::SendEchoData(int index_key, int data_size, uint8_t * data)
{
	bool result = false;

	auto object = Find(index_key);
	
	if (object != nullptr)
	{
		result = std::static_pointer_cast<TestTcpServerObject>(object)->SendEchoData(data_size, data);
	}

	return result;
}


