//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "udp_network_object.h"

//====================================================================================================
// Constructor 
// - is_send_data_lock : 단일 Thread 처리 에서는 false로 설정 사용
//====================================================================================================
UdpNetworkObject::UdpNetworkObject(bool is_send_data_lock/* = true*/)
{
	_recv_buffer = std::make_shared<std::vector<uint8_t>>(UDP_NETWORK_BUFFER_SIZE);
		
	_traffic_check_time = GetCurrentMilliSecond();

}

//====================================================================================================
// Destructor 
//====================================================================================================
UdpNetworkObject::~UdpNetworkObject()
{
	Release();

	// Post 종료 타이머 
	if (_post_close_timer != nullptr)
	{
		_post_close_timer = nullptr;
	}

	// KeepAlive 전송 타이머 정리 
	if (_keep_alive_send_timer != nullptr)
	{
		_keep_alive_send_timer = nullptr;
	}
	_keepalive_send_callback = nullptr;

	
}

//====================================================================================================
// 초기화 및 정리   
//====================================================================================================
void UdpNetworkObject::Release()
{
	if(_remote_end_point != nullptr)
	{
		_remote_end_point = nullptr;
	}
	
	if(_socket != nullptr)
	{
		_socket->close();
		_socket = nullptr;
	}

	_remote_ip_string = "";
	_socket = nullptr;
	_remote_end_point = nullptr;
	_is_create = false;
	
}

//====================================================================================================
// 생성  
//====================================================================================================
bool UdpNetworkObject::Create(UdpNetworkObjectParam *param)
{
	if (_is_create == true) return false;

	_object_key = param->object_key;
	_object_name = param->object_name;

	//Network Address -> host Address
	_remote_ip_string = boost::asio::ip::address_v4(boost::asio::detail::socket_ops::network_to_host_long(param->remote_ip)).to_string();

	_remote_end_point = std::make_shared<NetUdpEndPoint>(boost::asio::ip::address::from_string(_remote_ip_string.c_str()), 
														param->remote_port);
	
	_socket			= param->socket;
	_remote_ip		= param->remote_ip;
	_remote_port	= param->remote_port;
	_is_create		= true;
	_network_callback = param->network_callback;
	_object_callback = param->object_callback;
	

	StartReceive();
	
	return true;
}

//====================================================================================================
// 종료 설정 
// - 종료 
// - NetworkManager에서 호출
// - Linux에서 동일 Thread에서 종료 시켜야 정상 처리 예상 
//====================================================================================================
void UdpNetworkObject::PostClose()
{
	// 종료 진행 설정 
	_is_closeing = true;

	if (_post_close_timer != nullptr)
	{
		return;
	}

	_post_close_timer = std::make_shared<NetTimer>((boost::asio::io_context &)_socket->get_executor().context());

	_post_close_timer->expires_from_now(std::chrono::nanoseconds(DEFAULT_NETWORK_POST_CLOSE_TIMER_INTERVAL * 1000000));


	_post_close_timer->async_wait(boost::bind(&UdpNetworkObject::OnNetworkTimer,
		shared_from_this(),
		boost::asio::placeholders::error,
		_post_close_timer,
		(int)NetworkTimer::PostClose,
		DEFAULT_NETWORK_POST_CLOSE_TIMER_INTERVAL));
}

//====================================================================================================
// Post 종료 타이머 처리 
//====================================================================================================
bool UdpNetworkObject::PostCloseTimerProc()
{
	if (_keep_alive_send_timer != nullptr)	_keep_alive_send_timer->cancel(); 	// KeepAlive 전송 타이머 종료 
	if (_post_close_timer != nullptr)		_post_close_timer->cancel(); 		//Post 종료 타이머 종료 

	_socket->close();

	return true;
}

//====================================================================================================
// Timer설정 
//====================================================================================================
void UdpNetworkObject::SetNetworkTimer(std::shared_ptr<NetTimer> network_timer, int id, int interval)
{
	if (_is_closeing == true)
	{
		return;
	}


	network_timer->expires_from_now(std::chrono::nanoseconds(interval * 1000000));

	network_timer->async_wait(boost::bind(&UdpNetworkObject::OnNetworkTimer,
		shared_from_this(),
		boost::asio::placeholders::error,
		network_timer,
		id,
		interval));
}

//====================================================================================================
// Timer 처리 
//====================================================================================================
void UdpNetworkObject::OnNetworkTimer(const boost::system::error_code& error, std::shared_ptr<NetTimer> network_timer, int id, int interval)
{
	//종료 확인 
	if (_is_closeing == true)
	{
		if (id == (int)NetworkTimer::PostClose)
		{
			if (PostCloseTimerProc() == false)
			{
				// 타이머 재호출 
				SetNetworkTimer(network_timer, id, interval);
			}
		}

		return;
	}

	// 에러  
	if (error)
	{
		return;
	}

	switch (id)
	{
		case (int)NetworkTimer::KeepaliveSend :
		{
			// 함수 포인터 호출	
			if (_keepalive_send_callback != nullptr && (this->*_keepalive_send_callback)() == false)
			{
				return;
			}

			// 타이머 재호출 
			SetNetworkTimer(network_timer, id, interval);

			break;
		}

	}

	return;
}

//====================================================================================================
// KeepAlive 전송  Timer설정(Send/Check) 
// -UdpKeepaliveSendCallback 형변환 사용 
//    static_cast<bool (TcpNetworkObject::*)()>(&ClassName::CallbackFunction)
//    ex> SetKeepAliveSendTimer(n*1000, static_cast<bool (UdpNetworkObject::*)()>(&CServerObject::SendKeepAlive));
//====================================================================================================
void UdpNetworkObject::SetKeepAliveSendTimer(int interval, UdpKeepaliveSendCallback callback)
{
	if (_keep_alive_send_timer != nullptr)
	{
		_keep_alive_send_timer->cancel();
		_keep_alive_send_timer = nullptr;
	}

	_keep_alive_send_timer = std::make_shared<NetTimer>((boost::asio::io_context&)_socket->get_executor().context());
	_keepalive_send_callback = callback;
	SetNetworkTimer(_keep_alive_send_timer, (int)NetworkTimer::KeepaliveSend, interval);

}


//====================================================================================================
// 시작
//====================================================================================================
void UdpNetworkObject::StartReceive( )
{
	if(_is_create == false ) return;
	
	if (_is_closeing == false && _socket->is_open() == true)
	{
		_socket->async_receive_from(boost::asio::buffer(*_recv_buffer),
			*(_remote_end_point),
			boost::bind(&UdpNetworkObject::OnReceive,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
}

//====================================================================================================
// 패킷 전송 이벤트 설정(Data)
// - 다른 Thread에서 호출 가능 함수로 변수 주의 필요 
// - 전송 Data 최대 크기 확인( 0 - 무제한)  - 여러 개로 분할 하여 패킷 전송 
// - 전송 Queue 최대 크기 확인( 0- 무제한 )  - 에러 처리 
// - is_data_copy :  	true - 데이터 복사 (호출하는 곳에서 메모리 정리) - defalut  
//					false - 데이터 복사를 하지 않는다 포인터 그대로 전달 ( 호출 하는 곳에서 메모리 정리 필요 없음) 
//====================================================================================================
bool UdpNetworkObject::PostSend(std::shared_ptr<std::vector<uint8_t>> data, bool is_data_copy/*=false*/)
{
	// 종료 확인 
	if (_is_closeing == true)
	{
		LOG_INFO_WRITE(("[%s] UdpNetworkObject::PostSend - Closeing Return - key(%s) ip(%s)",
			_object_name.c_str(), _index_key, _remote_ip_string.c_str()));
		return false;
	}
					
	if (_is_closeing == false && _socket->is_open() == true)
	{
		_socket->async_send_to(boost::asio::buffer(*data),
			*_remote_end_point,
			boost::bind(&UdpNetworkObject::OnSend,
				shared_from_this(),
				data,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
			
	}

	return true;
}

//====================================================================================================
// 데이터 전송 완료 
//====================================================================================================
void UdpNetworkObject::OnSend(std::shared_ptr<std::vector<uint8_t>> const &data, const  NetErrorCode & error, size_t data_size)
{
	//에러 
	if (error)
	{
	
		LOG_ERROR_WRITE(("[%s] UdpNetworkObject::OnSend - key(%s) ip(%s) Error(%d) Message(%s)",
			_object_name.c_str(),
			_index_key,
			_remote_ip_string.c_str(),
			error.value(),
			error.message().c_str()));
		

		if (_is_closeing == false)
		{
			_is_closeing = true;

			//종료 콜백 호출 	
			if (_network_callback != nullptr)
				_network_callback->OnNetworkClose(_object_key, _index_key, _remote_ip, _remote_port);
		}

		return;
	}
	
	//Traffic Rate 확인 
	_send_traffic += data_size * 8;

	return;
}

//====================================================================================================
// Recv
//====================================================================================================
void UdpNetworkObject::OnReceive(const  NetErrorCode & error, size_t data_size)
{
	if (error)
	{
		LOG_ERROR_WRITE(("[%s] UdpNetworkObject::OnReceive - key(%s) ip(%s) Error(%d) Message(%s)",
					_object_name.c_str(),
					_index_key,
					_remote_ip_string.c_str(),
					error.value(),
					error.message().c_str()));

		if (_is_closeing == false)
		{
			_is_closeing = true;

			//종료 콜백 호출 	
			if (_network_callback != nullptr)
				_network_callback->OnNetworkClose(_object_key, _index_key, _remote_ip, _remote_port);
		}

	}

	//종료 확인 
	if (_is_closeing == true)
	{
			LOG_INFO_WRITE(("[%s] UdpNetworkObject::OnReceive Closeing Return - key(%s) ip(%s)",
						_object_name.c_str(), _index_key, _remote_ip_string.c_str()));
		

		return;
	}


	if (data_size > UDP_NETWORK_BUFFER_SIZE)
	{
		LOG_ERROR_WRITE(("[%s] OnReceive - error - remote(%s:%d) Error(%d) Message(%s)",
				_object_name.c_str(),
				_remote_ip_string.c_str(),
				_remote_port,
				error.value(),
				error.message().c_str()));
		return;
	}
  
	_recv_traffic += data_size * 8; // byte->bit

	_recv_buffer->resize(data_size);

	RecvHandler(_recv_buffer);
	
	_recv_buffer->resize(UDP_NETWORK_BUFFER_SIZE);

	StartReceive();
	
}

//====================================================================================================
// Send/Recv Traffic Rate
// - bps
//====================================================================================================
bool UdpNetworkObject::GetTrafficRate(double &send_bitrate, double &recv_bitrate)
{
	uint64_t current_time = GetCurrentMilliSecond();
	
	uint32_t check_gap = current_time - _traffic_check_time;
		
	if (check_gap < 1000)
	{
		return false;
	}

	send_bitrate = _send_traffic / check_gap * 1000;
	recv_bitrate = _recv_traffic / check_gap * 1000;
 
	_traffic_check_time = current_time; 
	_send_traffic = 0;
	_recv_traffic = 0;

	return true;
}


