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
		case NetworkObjectKey::RtmpEncoder:
			object_key_string = "RtmpEncoder";
			break;

		case NetworkObjectKey::HttpClient:
			object_key_string = "HttpClient";
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
	_rtmp_encoder_manager = std::make_shared<RtmpEncoderManager>((int)NetworkObjectKey::RtmpEncoder);
	_http_client_manager = std::make_shared<HttpClientManager>((int)NetworkObjectKey::HttpClient);

	_network_table[(int)NetworkObjectKey::RtmpEncoder] = _rtmp_encoder_manager;
	_network_table[(int)NetworkObjectKey::HttpClient] = _http_client_manager;

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
	
	// rtmp encoder
	if (!_rtmp_encoder_manager->Create(std::static_pointer_cast<INetworkCallback>(this->shared_from_this()),
										_network_context_pool,
										_create_param->rtmp_listen_port,
										GetNetworkObjectName(NetworkObjectKey::RtmpEncoder)))
	{
		LOG_WRITE(("ERROR : [%s] Create Fail", GetNetworkObjectName(NetworkObjectKey::RtmpEncoder).c_str()));
		return false;
	}

	// http client
	if (!_http_client_manager->Create(std::static_pointer_cast<INetworkCallback>(this->shared_from_this()),
									_network_context_pool,
									_create_param->http_listen_port,
									GetNetworkObjectName(NetworkObjectKey::HttpClient)))
	{
		LOG_WRITE(("ERROR : [%s] Create Fail", GetNetworkObjectName(NetworkObjectKey::HttpClient).c_str()));
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
	
	if (object_key == (int)NetworkObjectKey::RtmpEncoder)
	{
		_rtmp_encoder_manager->AcceptedAdd(socket,
										ip,
										port,
										std::static_pointer_cast<IRtmpEncoder>(this->shared_from_this()),
										index_key);
	}
	else if (object_key == (int)NetworkObjectKey::HttpClient)
	{
		_http_client_manager->AcceptedAdd(socket,
										ip,
										port,
										std::static_pointer_cast<IHttpClient>(this->shared_from_this()),
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
// OnRtmpEncoderStart
// - IRtmpEncoder Callback
//====================================================================================================
bool MainObject::OnRtmpEncoderStart(int index_key, uint32_t ip, StreamKey& stream_key)
{
	return true;
}

//====================================================================================================
// OnRtmpEncoderReadyComplete
// - IRtmpEncoder Callback
//====================================================================================================
bool MainObject::OnRtmpEncoderReadyComplete(int index_key, uint32_t ip, StreamKey& stream_key, MediaInfo& media_info)
{
	return true;
}

//====================================================================================================
// OnRtmpEncoderStreamData 
// - IRtmpEncoder Callback
//====================================================================================================
bool MainObject::OnRtmpEncoderStreamData(int index_key, uint32_t ip, StreamKey& stream_key, std::shared_ptr<FrameInfo>& frame_info)
{
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
