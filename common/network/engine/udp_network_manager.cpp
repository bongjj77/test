//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "../../common_header.h"
#include "udp_network_manager.h"


#define INDEX_KEY_MAX_SIZE (2000000000) //-2147483648 ~ 2147483647

//====================================================================================================
// Constructor 
//====================================================================================================
UdpNetworkManager::UdpNetworkManager(int object_key) : NetworkManager(object_key)
{
	_object_key 			= object_key; 
	_index_key 				= 0;
	_max_createing_count 	= 0;
	_network_info_map.clear();
	_listen_port			= 0; 
	_object_name			= "UnknownObject";
	_network_info_map.clear();
}

//====================================================================================================
// Destructor 
//====================================================================================================
UdpNetworkManager::~UdpNetworkManager( )
{
	LOG_WRITE(("[%s] UdpNetworkManager::~NetworkManager()", _object_name.c_str()));
}

//====================================================================================================
// 종료  
//====================================================================================================
void UdpNetworkManager::Release()
{
	
	LOG_WRITE(("[%s] UdpNetworkManager::Release()", _object_name.c_str()));

	RemoveAll(); 
}


//====================================================================================================
// 생성 
// - udp
//====================================================================================================
bool UdpNetworkManager::Create(INetworkCallback * callback, std::shared_ptr<NetworkContextPool> service_pool, int listen_port, std::string object_name)
{
	//Object 이름 저장 
	_object_name = object_name;

	//콜백 객체 저장 	
	_network_callback = callback;

	//IoService 설정
	_context_pool = service_pool;

	//서버 접속 생성( 0 = client)
	_listen_port = listen_port;

	return true;
}


//====================================================================================================
//삽입 
// - Key가 존재하는지 확인 
// - return -1 실패 
// - return n (IndexKey) 
//====================================================================================================
int UdpNetworkManager::Insert(std::shared_ptr<UdpNetworkObject> object, 
							bool is_keepalive_check/* = false*/, 
							uint32_t keepalive_check_time /*= 0*/)
{
	int index_key	= -1; 

	if(object == nullptr)
	{
		LOG_ERROR_WRITE(("[%s] UdpNetworkManager::Insert() Parameter nullptr", _object_name.c_str()));
		return -1;
	}

	
	std::lock_guard<std::mutex> network_info_map_lock(_network_info_map_mutex);;

	if(_index_key >= INDEX_KEY_MAX_SIZE)
	{
		_index_key = 0; 
	}

	// 인덱스키가 이미 사용 중이면 다음 인덱스 키 검색 
	if(_network_info_map.find(_index_key) != _network_info_map.end())
	{
		bool bFindIndex = false; 

		for(int index = _index_key; index < INDEX_KEY_MAX_SIZE ; index++)
		{
			if(_network_info_map.find(index) == _network_info_map.end())
			{
				_index_key = index;
				bFindIndex = true;
				break; 
			}
		}

		if(bFindIndex == false)
		{
			LOG_ERROR_WRITE(("[%s] UdpNetworkManager::Insert() IndexKey Over", _object_name.c_str()));
		}
	}

	// 삽입  
	if(_network_info_map.find(_index_key) == _network_info_map.end())
	{
		_network_info_map.insert(std::pair<int, std::shared_ptr<UdpNetworkObject>>(_index_key, object));

		// IndexKey 설정 
		index_key = _index_key; 

		// TcpNetworkObject에 IndexKey설정 
		object->SetIndexKey(_index_key);
		
		
		// 최대 개수 기록 
		if(_network_info_map.size() > _max_createing_count)
		{
			_max_createing_count = (uint32_t)_network_info_map.size();
		}
		
		// 에러 처리 
		if(index_key == -1)
		{
			_network_info_map.erase(_index_key);
		}
		
		// Index Key 증가 
		_index_key++; 
			
	}
	else 
	{
		LOG_ERROR_WRITE(("[%s] UdpNetworkManager::Insert() Exist IndexKey", _object_name.c_str()));
		index_key = -1; 
	}

	return index_key;
}

//====================================================================================================
// 검색(IP,Port)
//====================================================================================================
bool UdpNetworkManager::FindIndexKey(uint32_t ip, int port, int & index_key)
{
	bool					result 	= false;
	 
	std::lock_guard<std::mutex> network_info_map_lock(_network_info_map_mutex);;

	// 검색 
	for(const auto &item : _network_info_map)
	{
		auto object = item.second;

		if(object->GetRemoteIP() == ip && object->GetRemotePort() == port)
		{
			index_key 	= object->GetIndexKey();
			result		= true; 
			break; 
		}
	}

	return result; 
}

//====================================================================================================
// 연결 여부 (IP,Port)
//====================================================================================================
bool UdpNetworkManager::IsConnected(uint32_t ip, int port)
{
	int 	index_key 	= -1; 
	bool 	result 	= false; 
	
	result = FindIndexKey(ip, port,  index_key);
		
	return (result == true && index_key != -1)? true : false; 
}


//====================================================================================================
// 비동기 검색 
// - 사용하는 부분에서 동기화 처리  
//====================================================================================================
std::shared_ptr<UdpNetworkObject> UdpNetworkManager::Find(int index_key, bool erase/* = false*/)
{
	std::shared_ptr<UdpNetworkObject>	object;
	
	std::lock_guard<std::mutex> network_info_map_lock(_network_info_map_mutex);
	
	auto item = _network_info_map.find(index_key);
	if (item != _network_info_map.end())
	{
		object = item->second;

		if (erase)
			_network_info_map.erase(item);
	}

	return object;
}

//====================================================================================================
// 삭제
//====================================================================================================
bool UdpNetworkManager::Remove(int index_key)
{
	bool result = false;

	auto object = Find(index_key, true);

	if (object != nullptr)
	{
		//종료 요청 
		object->PostClose();
		result = true;
	}

	return result;
}

//====================================================================================================
// 삭제(목록) 
//====================================================================================================
void UdpNetworkManager::Remove(std::vector<int> IndexKeys)
{
	std::shared_ptr<UdpNetworkObject>	object;
	
	for (auto index_key : IndexKeys)
	{
		auto object = Find(index_key, true);

		if (object != nullptr)
		{
			//종료 요청 
			object->PostClose();
		}
	}
	
	return; 
}

//====================================================================================================
// 전체 삭제   
// - 연결 종료 
//====================================================================================================
void UdpNetworkManager::RemoveAll()
{
	std::shared_ptr<UdpNetworkObject>	object;

	std::lock_guard<std::mutex> network_info_map_lock(_network_info_map_mutex);;

	// 삭제
	for(auto item = _network_info_map.begin() ; item != _network_info_map.end(); )
	{
		object = item->second;

		//삭제 
		_network_info_map.erase(item++);
	}

	_network_info_map.clear(); 

	return;
}

//====================================================================================================
// 연결 개수 
//====================================================================================================
uint32_t UdpNetworkManager::GetCount()
{
	uint32_t count = 0;

	std::unique_lock<std::mutex> network_info_map_lock(_network_info_map_mutex);;

	count = (uint32_t)_network_info_map.size();

	return count;
}

//====================================================================================================
// 네트워크 전체 트래픽  
// - bps 
//====================================================================================================
uint64_t UdpNetworkManager::GetTotalTrafficRate()
{
 	uint64_t traffic_rate = 0; 
	double send_bitrate; 
	double recv_bitrate; 
	
	std::lock_guard<std::mutex> network_info_map_lock(_network_info_map_mutex);;

	for (const auto &item : _network_info_map)
	{
		auto object = item.second;

		if (object->GetTrafficRate(send_bitrate, recv_bitrate))
		{
			traffic_rate += (send_bitrate + recv_bitrate);
		}
	}

 	return traffic_rate;
	
}