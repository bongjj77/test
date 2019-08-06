#include "main_object.h"
#include "openssl/ssl.h"
#include <iomanip>

enum
{
	
};

//===============================================================================================
// GetNetworkObjectName
//===============================================================================================
std::string MainObject::GetNetworkObjectName(NetworkObjectKey object_key)
{
	std::string object_key_string;

	switch (object_key)
	{
	
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
	_start_time = time(nullptr);

}

//====================================================================================================
// Destructor
//====================================================================================================
MainObject::~MainObject( )
{
	Destroy();
}

//====================================================================================================
// 종료 
//====================================================================================================
void MainObject::Destroy( )
{
	_thread_timer.Destroy();

	//종료 
	for(int index = 0; index < (int)NetworkObjectKey::Max ; index++)
	{
		_network_table[index]->PostRelease(); 
	}
	
	//서비스 종료
	if(_network_service_pool != nullptr)
	{
		_network_service_pool->Stop(); 

		LOG_WRITE(("INFO : Network Service Pool Stop Completed"));
	}
}

//====================================================================================================
// 생성
//====================================================================================================
bool MainObject::Create(CreateParam *param)
{  
	if(param == nullptr)
    {
		LOG_WRITE(("ERROR : Create Param Fail"));
		return false;
    }

	// Real Host  설정 
	std::string host_name; 

	if(GetLocalHostName(host_name) == true)
	{
		strcpy(param->real_host_name, host_name.c_str());
	}
	else
	{
		strcpy(param->real_host_name, param->host_name);
	}

	LOG_WRITE(("INFO : Host Name - %s(%s) ", param->host_name, param->real_host_name));
	
	memcpy(&_create_param, param, sizeof(CreateParam));

	// IoService 실행 
	_network_service_pool = std::make_shared<NetworkContextPool>(param->thread_pool_count);
	_network_service_pool->Run();  	
		
	// Timer 생성 	
	if(	_thread_timer.Create(this) == false)
	{
		LOG_WRITE(("ERROR : Init Timer Fail"));
	    return false;   
	}
	
	return true;
}

//====================================================================================================
// Network Accepted 콜백
//====================================================================================================
bool MainObject::OnTcpNetworkAccepted(int object_key, NetTcpSocket * & socket, uint32_t ip, int port)
{
	if(object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_WRITE(("ERRRO : OnTcpNetworkAccepted - Unkown ObjectKey - Key(%d)", object_key));
		return false; 
	}
		
	int index_key = -1; 
		
	// if (object_key == xxx)		_xxx.AcceptedAdd(socket, ip, port, this, _create_param.debug_mode, index_key);

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
// Network Connected 콜백
//====================================================================================================
bool MainObject::OnTcpNetworkConnected(int object_key, 
									NetConnectedResult result, 
									std::shared_ptr<std::vector<uint8_t>> connected_param, 
									NetTcpSocket * socket, 
									unsigned ip, 
									int port)
{
	if(object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_WRITE(("ERRRO : OnTcpNetworkConnected - Unkown ObjectKey - Key(%d)", object_key));
		return false; 
	}
	
	/*
	if (object_key == (int)NetworkObjectKey::SignallingServer)
		SignallingServerConnectedProc(result, (SignallingServerConnectedParam*)connected_param->data(), socket, ip, port);
	else
		LOG_WRITE(("INFO : [%s] OnTcpNetworkConnected - Unknown Connect Object -IP(%s) Port(%d)", 
			GetNetworkObjectName((NetworkObjectKey)object_key).c_str(), 
			GetStringIP(ip).c_str(), 
			port));
	
	*/
	

	return true;
}

//====================================================================================================
// Network ConnectedSSL 콜백
//====================================================================================================
bool MainObject::OnTcpNetworkConnectedSSL(int object_key,
	NetConnectedResult result,
	std::shared_ptr<std::vector<uint8_t>> connected_param,
	NetSocketSSL * socket,
	unsigned ip,
	int port)
{
	if (object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_WRITE(("ERRRO : OnTcpNetworkConnectedSSL - Unkown ObjectKey - Key(%d)", object_key));
		return false;
	}

	/*
	if (object_key == (int)NetworkObjectKey::WowzaSignallingServer)
	WowzaSignallingServerConnectedProc(result, (SignallingServerConnectedParam*)connected_param->data(), socket, ip, port);
	else
	LOG_WRITE(("INFO : [%s] OnTcpNetworkConnectedSSL - Unknown Connect Object -IP(%s) Port(%d)",
	GetNetworkObjectName((NetworkObjectKey)object_key).c_str(),
	GetStringIP(ip).c_str(),
	port));
	*/
	

	return true;
}

//====================================================================================================
// Network 연결 종료 콜백(연결 종료)  
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
	
	/*
	// TODO : 정보 처러 완료 상태에서는 스트림 제거 필요 없음 steream_manager 객체에서 상태갑 확인 절차 차후 구현 
	if (object_key == (int)NetworkObjectKey::SignallingServer) 
		xxx.GetStreamKey(index_key, stream_key);
	*/
	
	
	//삭제 
	_network_table[object_key]->Remove(index_key); 
		
	return 0;
}

//====================================================================================================
// 소켓 연결 삭제 
// - 스트림 제거시 호출 
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
// 소켓 연결 삭제 
// - 스트림 제거시 호출 
// - 인덱스키 배열 
//====================================================================================================
bool MainObject::RemoveNetwork(int object_key, std::vector<int> & IndexKeys) 
{
	if(object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_WRITE(("ERRRO : RemoveNetwork - ObjectKey(%d)", object_key));
		return false; 
	}

	//삭제 
	_network_table[object_key]->Remove(IndexKeys); 

	return true; 
}

//====================================================================================================
// 기본 타이머 Callback
//====================================================================================================
void MainObject::OnThreadTimer(uint32_t timer_id, bool &delete_timer/* = false */)
{
	switch(timer_id)
	{
	
	}
}
