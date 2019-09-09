//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "main_object.h"
#include <iomanip>

enum class Timer
{
	StartTest,
	 
};

#define START_TEST_TIMER_INTERVAL	(5)

//===============================================================================================
// GetNetworkObjectName
//===============================================================================================
std::string MainObject::GetNetworkObjectName(NetworkObjectKey object_key)
{
	std::string object_key_string;

	switch (object_key)
	{
		case NetworkObjectKey::TestTcpServer:
			object_key_string = "TestTcpServer";
			break;	

		default:
			object_key_string = "unknown";
			break;
	}

	return object_key_string;
}

//====================================================================================================
// Constructor
//====================================================================================================
MainObject::MainObject( )
{
	_test_tcp_server_manager = std::make_shared<TestTcpServerManager>((int)NetworkObjectKey::TestTcpServer);

	_network_table[(int)NetworkObjectKey::TestTcpServer] = _test_tcp_server_manager;
}

//====================================================================================================
// Destructor
//====================================================================================================
MainObject::~MainObject( )
{
	Destroy();
}

//====================================================================================================
// Destroy 
//====================================================================================================
void MainObject::Destroy( )
{
	_timer.Destroy();

	// network close
	for(int index = 0; index < (int)NetworkObjectKey::Max ; index++)
	{
		_network_table[index]->PostRelease(); 		
	}
	LOG_INFO_WRITE(("Network Object Close Completed"));

	// network service close
	if(_network_context_pool != nullptr)
	{
		_network_context_pool->Stop(); 

		LOG_INFO_WRITE(("Network Service Pool Close Completed"));
	}
}

//====================================================================================================
// Create
//====================================================================================================
bool MainObject::Create(std::unique_ptr<CreateParam> create_param)
{  
	_create_param = std::move(create_param);
	 
	// IoService start
	_network_context_pool = std::make_shared<NetworkContextPool>(_create_param->thread_pool_count);
	_network_context_pool->Run();  	
		
	// create test tcp client
	if (!_test_tcp_server_manager->Create(std::static_pointer_cast<ITcpNetwork>(this->shared_from_this()), 
										_network_context_pool, 
										GetNetworkObjectName(NetworkObjectKey::TestTcpServer)))
	{
		LOG_ERROR_WRITE(("[%s] Create Fail", GetNetworkObjectName(NetworkObjectKey::TestTcpServer).c_str()));
		return false;
	}
	 
	// create timer 	
	if(_timer.Create(this) == false || 
		_timer.SetTimer((uint32_t)Timer::StartTest, START_TEST_TIMER_INTERVAL) == false)
	{
		LOG_ERROR_WRITE(("Init Timer Fail"));
	    return false;   
	}
	
	return true;
}

//====================================================================================================
// Network Accepted Callback
//====================================================================================================
bool MainObject::OnTcpNetworkAccepted(int object_key, std::shared_ptr<NetTcpSocket> socket, uint32_t ip, int port)
{
	return true; 
}

//====================================================================================================
// Network Connected Callback
//====================================================================================================
bool MainObject::OnTcpNetworkConnected(	int object_key, 
										NetConnectedResult result, 
										std::shared_ptr<std::vector<uint8_t>> connected_param, 
										std::shared_ptr<NetTcpSocket> socket,
										unsigned ip, 
										int port)
{
	if (object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_ERROR_WRITE(("OnTcpNetworkConnected - Unkown ObjectKey - Key(%d)", object_key));
		return false;
	}

	if (object_key == (int)NetworkObjectKey::TestTcpServer)
	{
		TestTcpServerConnectedProc(result, (TestTcpServerConnectedParam*)connected_param->data(), socket, ip, port);
	}
	else
	{
		LOG_INFO_WRITE(("[%s] OnTcpNetworkConnected - Unknown Connect Object -IP(%s) Port(%d)",
					GetNetworkObjectName((NetworkObjectKey)object_key).c_str(),
					GetStringIP(ip).c_str(),
					port));
	}
		

	return true;
}

//====================================================================================================
// Network Connection Close Callback
//====================================================================================================
int MainObject::OnNetworkClose(int object_key, int index_key, uint32_t ip, int port)
{
	int stream_key = -1;
			
	if(object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_ERROR_WRITE(("OnNetworkClose - Unkown ObjectKey - Key(%d)", object_key));
		return 0; 
	}

	LOG_INFO_WRITE(("[%s] OnNetworkClose - IndexKey(%d) IP(%s) Port(%d)", 
		GetNetworkObjectName((NetworkObjectKey)object_key).c_str(), 
		index_key, 
		GetStringIP(ip).c_str(), 
		port));
	
	// remove session 
	_network_table[object_key]->Remove(index_key); 
		
	return 0;
}

//====================================================================================================
// Delete Session 
//====================================================================================================
bool MainObject::RemoveNetwork(int object_key, int index_key) 
{
	if(object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_ERROR_WRITE(("RemoveNetwork - ObjectKey(%d)", object_key));
		return false; 
	}

	//삭제 
	_network_table[object_key]->Remove(index_key); 

	return true; 
	
}

//====================================================================================================
// Delete Sessions 
//====================================================================================================
bool MainObject::RemoveNetwork(int object_key, std::vector<int> & IndexKeys) 
{
	if(object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_ERROR_WRITE(("RemoveNetwork - ObjectKey(%d)", object_key));
		return false; 
	}

	// Remove
	_network_table[object_key]->Remove(IndexKeys); 

	return true; 
}


//====================================================================================================
// SignallingServer 연결 이후 처리 
//====================================================================================================
bool MainObject::TestTcpServerConnectedProc(NetConnectedResult result_code,
											TestTcpServerConnectedParam* connected_param,
											std::shared_ptr<NetTcpSocket> socket,
											uint32_t ip,
											int port)
{
	int index_key = -1;

	if (result_code != NetConnectedResult::Success || socket == nullptr)
	{
		LOG_ERROR_WRITE(("TestTcpServerConnectedProc - Connected Result Fail - TestTcpServer(%s:%d)", GetStringIP(ip).c_str(), port));

		return false;
	}

	// Session add
	if (_test_tcp_server_manager->ConnectedAdd(	socket, 
												std::static_pointer_cast<ITestTcpServer>(this->shared_from_this()),
												index_key) == false)
	{
		LOG_ERROR_WRITE(("TestTcpServerConnectedProc - ConnectAdd Fail - TestTcpServer(%s:%d)", GetStringIP(ip).c_str(), port));
		return false;
	}

	// data send
	std::string data = "Echo Test";
	if (_test_tcp_server_manager->SendEchoData(index_key, data.size() + 1, (uint8_t *)data.c_str()) == false)
	{
		LOG_ERROR_WRITE(("TestTcpServerConnectedProc - SendEchoData Fail - TestTcpServer(%s:%d)", GetStringIP(ip).c_str(), port));
		return false;
	}

	return true;
}


//====================================================================================================
// Timerr Callback
//====================================================================================================
void MainObject::OnThreadTimer(uint32_t timer_id, bool &delete_timer/* = false */)
{
	switch(timer_id)
	{
		case (uint32_t)Timer::StartTest :
		{
			StartTestTimeProc();

			delete_timer = true; 

			break;
		}
	}
}

//====================================================================================================
// 시작 테스트 처리 
// - 1회 실행
//====================================================================================================
void MainObject::StartTestTimeProc()
{
	auto connected_param = std::make_shared<std::vector<uint8_t>>(sizeof(TestTcpServerConnectedParam));
	
	TestTcpServerConnectedParam *test_tcp_server_param = (TestTcpServerConnectedParam *)connected_param->data();
	test_tcp_server_param->index = 0;

	_test_tcp_server_manager->PostConnect(	_create_param->test_tcp_server_ip, 
											_create_param->test_tcp_server_port, 
											connected_param);
}
