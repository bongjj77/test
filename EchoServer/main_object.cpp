//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "main_object.h"
#include "openssl/ssl.h"
#include <iomanip>

//===============================================================================================
// GetNetworkObjectName
//===============================================================================================
std::string MainObject::GetNetworkObjectName(NetworkObjectKey object_key)
{
	std::string object_key_string;

	switch (object_key)
	{
		case NetworkObjectKey::TestTcpClient:
			object_key_string = "TestTcpClient";
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
MainObject::MainObject()
{
	_test_tcp_client_manager = std::make_shared<TestTcpClintManager>((int)NetworkObjectKey::TestTcpClient);

	_network_table[(int)NetworkObjectKey::TestTcpClient] = _test_tcp_client_manager;
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
	LOG_WRITE(("INFO : Network Object Close Completed"));

	// network service close
	if(_network_context_pool != nullptr)
	{
		_network_context_pool->Stop(); 

		LOG_WRITE(("INFO : Network Service Pool Close Completed"));
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
	if (!_test_tcp_client_manager->Create(std::static_pointer_cast<INetworkCallback>(this->shared_from_this()),
										_network_context_pool, 
										_create_param->test_tcp_client_listen_port, 
										GetNetworkObjectName(NetworkObjectKey::TestTcpClient)))
	{
		LOG_WRITE(("ERROR : [%s] Create Fail", GetNetworkObjectName(NetworkObjectKey::TestTcpClient).c_str()));
		return false;
	}
	 
	// create timer 	
	if(	_timer.Create(this) == false)
	{
		LOG_WRITE(("ERROR : Init Timer Fail"));
	    return false;   
	}
	
	return true;
}

//====================================================================================================
// Network Accepted Callback
//====================================================================================================
bool MainObject::OnTcpNetworkAccepted(int object_key, std::shared_ptr<NetTcpSocket> socket, uint32_t ip, int port)
{
	if(object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_WRITE(("ERRRO : OnTcpNetworkAccepted - Unkown ObjectKey - Key(%d)", object_key));
		return false; 
	}
		
	int index_key = -1; 
		
	if (object_key == (int)NetworkObjectKey::TestTcpClient)
	{
		_test_tcp_client_manager->AcceptedAdd(socket, 
											ip, 
											port, 
											std::static_pointer_cast<ITestTcpClientCallback>(this->shared_from_this()), 
											index_key);
	}
	
	if(index_key == -1)
	{
		LOG_WRITE(("ERRRO : [%s] OnTcpNetworkAccepted - Object Add Fail - IndexKey(%d) IP(%s)", 
					GetNetworkObjectName((NetworkObjectKey)object_key).c_str(), 
					index_key, 
					GetStringIP(ip).c_str()));
		return false; 
	}
	
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
	if(object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_WRITE(("ERRRO : OnTcpNetworkConnected - Unkown ObjectKey - Key(%d)", object_key));
		return false; 
	}
	
	return true;
}

//====================================================================================================
// Network ConnectedSSL Callback
//====================================================================================================
bool MainObject::OnTcpNetworkConnectedSSL(int object_key,
										NetConnectedResult result,
										std::shared_ptr<std::vector<uint8_t>> connected_param,
										std::shared_ptr < NetSocketSSL> socket,
										unsigned ip,
										int port)
{
	if (object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_WRITE(("ERRRO : OnTcpNetworkConnectedSSL - Unkown ObjectKey - Key(%d)", object_key));
		return false;
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
		LOG_WRITE(("ERRRO : OnNetworkClose - Unkown ObjectKey - Key(%d)", object_key));
		return 0; 
	}

	LOG_WRITE(("INFO : [%s] OnNetworkClose - IndexKey(%d) IP(%s) Port(%d)", 
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
		LOG_WRITE(("ERRRO : RemoveNetwork - ObjectKey(%d)", object_key));
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
		LOG_WRITE(("ERRRO : RemoveNetwork - ObjectKey(%d)", object_key));
		return false; 
	}

	// Remove
	_network_table[object_key]->Remove(IndexKeys); 

	return true; 
}

//====================================================================================================
// Timerr Callback
//====================================================================================================
void MainObject::OnThreadTimer(uint32_t timer_id, bool &delete_timer/* = false */)
{
	switch(timer_id)
	{
	
	}
}
