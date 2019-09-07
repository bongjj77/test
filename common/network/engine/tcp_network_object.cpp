//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "../../common_header.h"
#include "tcp_network_object.h"
#include "network_manager.h" 
#include <iostream>
#include <stdarg.h>

//====================================================================================================
// Constructor 
//====================================================================================================
TcpNetworkObject::TcpNetworkObject() 
{
	_recv_buffer				= std::make_shared<std::vector<uint8_t>>(DEAUULT_SOCKET_BUFFER_SZIE, 0);
	_create_time				= time(nullptr);
	_traffic_check_time			= GetCurrentMilliSecond();
	_max_send_data_size			= DEFAULT_MAX_WAIT_SEND_DATA_SIZE;
	_post_close_time_interval 	= DEFAULT_NETWORK_POST_CLOSE_TIMER_INTERVAL;	
}

//====================================================================================================
// Destructor 
//====================================================================================================
TcpNetworkObject::~TcpNetworkObject()
{
	if(_close_timer != nullptr)
	{
		_close_timer = nullptr; 
	}

	if(_post_close_timer != nullptr)
	{
		_post_close_timer = nullptr; 
	}
		
	if(_keep_alive_send_timer != nullptr)
	{
		_keep_alive_send_timer = nullptr; 
	}
	
	_keepalive_send_callback = nullptr; 

	if(_keepalive_check_timer != nullptr)
	{
		_keepalive_check_timer->cancel(); 	
		_keepalive_check_timer = nullptr; 
	}
	
	_keepalive_check_callbak = nullptr; 
	
	ClearSendDataQueue(); 

	if(_socket != nullptr)
	{
		_socket = nullptr; 
	}
 
	_network_callback 	= nullptr; 
	_object_callback = nullptr; 

}

//====================================================================================================
// Create  
//====================================================================================================
bool TcpNetworkObject::Create(TcpNetworkObjectParam *param)
{
	if(param->socket == nullptr)
	{
		return false; 
	}

	_socket = param->socket;
	_object_key = param->object_key;
	_object_name = param->object_name;

	try
	{
		_remote_ip_string = _socket->remote_endpoint().address().to_v4().to_string();
		_remote_ip = boost::asio::detail::socket_ops::network_to_host_long(_socket->remote_endpoint().address().to_v4().to_ulong()); 
		_remote_port = _socket->remote_endpoint().port();			
	}
	catch (const std::exception& e)
	{
		LOG_ERROR_WRITE(("[%s] TcpNetworkObject::Create - Socket Exception - ip(%s) Error(%s)", 
					param->object_name, _remote_ip_string.c_str(), e.what())); 

		return false;
	}
	
	_network_callback	= param->network_callback;
	_object_callback	= param->object_callback;
	_create_time		= time(nullptr);

	return true;

}


//====================================================================================================
// IsOpened  
//====================================================================================================
bool TcpNetworkObject::IsOpened()
{
	return _socket->is_open();
}

//====================================================================================================
// SocketClose  
//====================================================================================================
void TcpNetworkObject::SocketClose()
{
	_socket->close();
}

//====================================================================================================
// GetIoContext  
//====================================================================================================
boost::asio::io_context& TcpNetworkObject::GetIoContext()
{
	return (boost::asio::io_context&)_socket->get_executor().context();
};

//====================================================================================================
// AsyncRead  
//====================================================================================================
void TcpNetworkObject::AsyncRead()
{
	_socket->async_read_some(boost::asio::buffer(*_recv_buffer),
		boost::bind(&TcpNetworkObject::OnReceive,
			shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

//====================================================================================================
// AsyncWrite  
//====================================================================================================
void TcpNetworkObject::AsyncWrite(std::shared_ptr<std::vector<uint8_t>> data)
{
	boost::asio::async_write(*_socket,
		boost::asio::buffer(*(data)),
		boost::bind(&TcpNetworkObject::OnSend,
			shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}



//====================================================================================================
// 전송 데이터 큐 정리 
//====================================================================================================
void TcpNetworkObject::ClearSendDataQueue()
{

	//동기화 시작 
	std::lock_guard<std::mutex> send_data_lock(_send_data_queue_mutex);

	//전송 데이터 정리 
	_send_data_queue.clear();  
	
}

//====================================================================================================
// 시작 - 패킷 수신 이벤트  설정 
//====================================================================================================
bool TcpNetworkObject::Start()
{
	if(IsOpened() == false)
	{
		LOG_ERROR_WRITE(("[%s] TcpNetworkObject::Start SocketClose - key(%s) ip(%s)", 
					_object_name.c_str(), _index_key, _remote_ip_string.c_str()));
		return false; 
	}
	
	AsyncRead();

	return true; 
}

//====================================================================================================
// 종료 설정 
// - 종료 
// - NetworkManager에서 호출
// - Linux에서 동일 Thread에서 종료 시켜야 정상 처리 예상 
//====================================================================================================
void TcpNetworkObject::PostClose()
{
	// 종료 진행 설정 
	_is_closeing = true; 
	
	if(_post_close_timer != nullptr)
	{
		return; 
	}
	
	_post_close_timer = std::make_shared<NetTimer>(GetIoContext());
	
	_post_close_timer->expires_from_now(std::chrono::nanoseconds(_post_close_time_interval * 1000000));

	_post_close_timer->async_wait(boost::bind(&TcpNetworkObject::OnNetworkTimer, 
		shared_from_this(), 
		boost::asio::placeholders::error, 
		_post_close_timer, 
		(int)NetworkTimer::PostClose,
		_post_close_time_interval));
}

//====================================================================================================
// 패킷 전송 이벤트 설정(Data)
// - 다른 Thread에서 호출 가능 함수로 변수 주의 필요 
// - 전송 Data 최대 크기 확인( 0 - 무제한)  - 여러 개로 분할 하여 패킷 전송 
// - 전송 Queue 최대 크기 확인( 0- 무제한 )  - 에러 처리 
// - is_data_copy :  	true - 데이터 복사 (호출하는 곳에서 메모리 정리) - defalut  
//					false - 데이터 복사를 하지 않는다 포인터 그대로 전달 ( 호출 하는 곳에서 메모리 정리 필요 없음) 
//====================================================================================================
bool TcpNetworkObject::PostSend(std::shared_ptr<std::vector<uint8_t>> data, bool is_data_copy/*=false*/) 
{
	
	if(data == nullptr || data->size() == 0)
	{
		LOG_ERROR_WRITE(("[%s] TcpNetworkObject::PostSend - Param Fail - key(%s) ip(%s)", 
					_object_name.c_str(), _index_key, _remote_ip_string.c_str()));
		return false; 
	}

	
	// 종료 확인 
	if(_is_closeing == true)
	{
		LOG_INFO_WRITE(("[%s] TcpNetworkObject::PostSend - Closeing Return - key(%s) ip(%s)", 
					_object_name.c_str(), _index_key, _remote_ip_string.c_str()));
		return false; 
	}

	// Send Queue 초과 확인( 0 - 무제한) 
	if(_max_send_data_size > 0 && (_max_send_data_size < data->size()))
	{
		
		LOG_ERROR_WRITE(("[%s] TcpNetworkObject::PostSend - MaxWaitSendDataSize Over - key(%s) ip(%s) Size(%d:%d)", 
				_object_name.c_str(),
				_index_key, 
				_remote_ip_string.c_str(),
				data->size(),
				_max_send_data_size));
		
		return false; 
	}

	auto send_data = std::make_shared<NetSendData>();

	send_data->sned_time = 0;

	if (is_data_copy) 
		send_data->data = std::make_shared<std::vector<uint8_t>>(data->begin(), data->end()); 
	else	
		send_data->data	= data; 			
		
	//동기화 시작 
	std::lock_guard<std::mutex> send_data_lock(_send_data_queue_mutex);

	_send_data_queue.push_back(send_data);

	//데이터 처리 존재 하지 않은 상황에서 이벤트 생성
	if(_send_data_queue.size() == 1 && _is_closeing == false)
	{
		auto send_data = _send_data_queue.front();
				
		if(IsOpened() == true)
		{
			send_data->sned_time = time(nullptr);

			AsyncWrite(send_data->data);
		}	
	}
	
	return true; 
}

//====================================================================================================
//패킷 헤더 읽기 완료 
//데이터 처리 상속 구현 
// - Return < 0 이하 - 오류  
// - Return	= 0 - 데이터 부족(정상)  
// - Return	> 0	- 처리 데이터 크기(정상)   	
// - 상속 받은 곳에서 멀티패킷 처리 및 남은 데이터 저장 
//====================================================================================================
void TcpNetworkObject::OnReceive(const  NetErrorCode & error, size_t data_size)
{
	if(error)
	{
		
		//if(error == boost::asio::error::eof)
		//{
		//	_socket->shutdown(tcp::socket::shutdown_both);
		//}


	//	if ((error.value() & 0xff) == SSL_R_SHORT_READ)
	//	{
//
	//		LOG_INFO_WRITE(("[%s] TcpNetworkObject::OnReceive - SSL Shot Read  - key(%s) ip(%s)", _object_name.c_str(), _index_key, _remote_ip_string.c_str()));	
	//		
	//		return; 
			
		
	//	}
		
		_network_error = true; 
		
		if(_log_lock == false)
		{
			LOG_ERROR_WRITE(("[%s] TcpNetworkObject::OnReceive - key(%s) ip(%s) Error(%d) Message(%s)", 
						_object_name.c_str(), 
						_index_key, 
						_remote_ip_string.c_str(), 
						error.value(), 
						error.message().c_str()));
		}

		if(_is_closeing == false)
		{		
			_is_closeing = true; 

			//종료 콜백 호출 	
			if(_network_callback != nullptr)	
				_network_callback->OnNetworkClose(_object_key, _index_key, _remote_ip, _remote_port);
		}
		
		return;
	}

	//종료 확인 
	if(_is_closeing == true || _network_error == true)
	{
		if(_log_lock == false)
		{
			LOG_INFO_WRITE(("[%s] TcpNetworkObject::OnReceive Closeing Return - key(%s) ip(%s)", 
						_object_name.c_str(), _index_key, _remote_ip_string.c_str()));
		}

		return; 
	}
		
	time_t current_time	= 0;
	int time_gap = 0; 
	
	// rest data copy
	if(_rest_data != nullptr && _rest_data->size() > 0)
	{
		_recv_data = _rest_data;
		_rest_data = nullptr;

		_recv_data->insert(_recv_data->end(), _recv_buffer->begin(), _recv_buffer->begin() + data_size);
	}
	else
	{
		_recv_data = std::make_shared<std::vector<uint8_t>>(_recv_buffer->begin(), _recv_buffer->begin() + data_size);
	}

	int process_size = RecvHandler(_recv_data);
	
	  
	if(process_size < 0 || process_size > _recv_data->size())
	{
		LOG_ERROR_WRITE(("[%s] TcpNetworkObject::OnReceive - RecvHandler - key(%s) ip(%s) Result(%d)", 
					_object_name.c_str(), _index_key, _remote_ip_string.c_str(), process_size));

		//종료 콜백 호출 
		if(_is_closeing == false)
		{
			_is_closeing = true; 

			if(_network_callback != nullptr)
				_network_callback->OnNetworkClose(_object_key, _index_key, _remote_ip, _remote_port);
		}

		return;
	}

	// rest data save
	int rest_data_size = (int)_recv_data->size() - process_size;

	if(rest_data_size > 0)
		_rest_data = std::make_shared<std::vector<uint8_t>>(_recv_data->data() + process_size,
															_recv_data->data() + process_size + rest_data_size);
		
	

	
	if(_is_closeing == false && IsOpened() == true)
	{
		AsyncRead();
	}
		
	//Traffic Rate 
	current_time	= time(nullptr); 
	_recv_traffic 	+= process_size*8;	
	
	return; 
}

//====================================================================================================
// Data Send Complete
// - next data send from queue
//
//====================================================================================================
void TcpNetworkObject::OnSend(const  NetErrorCode & error, size_t data_size)
{	
	bool	is_complete_close 	= false; 	
	time_t	current_time		= 0;
	int  	time_gap 			= 0; 

	//에러 
	if(error)
	{
		//if(error == boost::asio::error::eof)
		//{
		//	_socket->shutdown(tcp::socket::shutdown_both);
		//}

		
		_network_error = true; 

		if(_log_lock == false)
		{
			LOG_ERROR_WRITE(("[%s] TcpNetworkObject::OnSend - key(%s) ip(%s) Error(%d) Message(%s)", 
						_object_name.c_str(),
						_index_key, 
						_remote_ip_string.c_str(), 
						error.value(),
						error.message().c_str()));
		}

		if(_is_closeing == false)
		{		
			_is_closeing = true; 

			//종료 콜백 호출 	
			if(_network_callback != nullptr)
				_network_callback->OnNetworkClose(_object_key, _index_key, _remote_ip, _remote_port);
		}
		
		return; 
	}

	if(_is_closeing == true || _network_error == true)
	{
		if(_log_lock == false)
		{
			LOG_INFO_WRITE(("[%s] TcpNetworkObject::OnSend Closeing Return - key(%s) ip(%s)", 
						_object_name.c_str(), _index_key, _remote_ip_string.c_str()));
		}

		return;
	}
	
	// Send Queue Sync Start
	std::lock_guard<std::mutex> send_data_lock(_send_data_queue_mutex);
		
	if(_send_data_queue.empty() == false)
	{
		_last_send_complete_time = time(nullptr); 
		
		if(_is_send_completed_close == true)
		{
			is_complete_close = true; 
		}

		_send_data_queue.pop_front(); 
 
		if(_send_data_queue.empty() == false)
		{
 	
			is_complete_close = false; 
			
			if(_is_closeing == false)
			{
				auto send_data = _send_data_queue.front();
								
				if(send_data->data != nullptr && IsOpened() == true)
				{
					send_data->sned_time = time(nullptr); 

					AsyncWrite(send_data->data);
				}
				
				
			}
		}
	}
	
	if(is_complete_close == true)
	{
		SetCloseTimer();
	}

	//Traffic Rate
	_send_traffic += data_size *8;
	
	return;
}

//====================================================================================================
// KeepAlive Send Timer Setting(Send/Check) 
// -TcpKeepaliveSendCallback 형변환 사용 
//    static_cast<bool (TcpNetworkObject::*)()>(&ClassName::CallbackFunction)
//    ex> SetKeepAliveSendTimer(n*1000, static_cast<bool (TcpNetworkObject::*)()>(&CServerObject::SendKeepAlive));
//====================================================================================================
void TcpNetworkObject::SetKeepAliveSendTimer(int interval, TcpKeepaliveSendCallback callback)
{
	if(_keep_alive_send_timer != nullptr)
	{
		_keep_alive_send_timer->cancel(); 
		_keep_alive_send_timer = nullptr; 
	}
	
	_keep_alive_send_timer = std::make_shared<NetTimer>(GetIoContext());

	_keepalive_send_callback 	= callback; 
	SetNetworkTimer(_keep_alive_send_timer, (int)NetworkTimer::KeepaliveSend, interval);
	
} 

//====================================================================================================
// Keepalive Check TImer Setting 
//====================================================================================================
void TcpNetworkObject::SetKeepAliveCheckTimer(int interval, TcpKeepAliveCheckCallback callback)
{
	if(_keepalive_check_timer != nullptr)
	{
		_keepalive_check_timer->cancel(); 
		_keepalive_check_timer = nullptr; 
	}
	
	_keepalive_check_timer = std::make_shared<NetTimer>(GetIoContext());
	_keepalive_check_callbak 	= callback; 
	SetNetworkTimer(_keepalive_check_timer, (int)NetworkTimer::KeepaliveCheck, interval);
}


//====================================================================================================
// 종료 Timer설정
//====================================================================================================
void TcpNetworkObject::SetCloseTimer()
{
	if(_close_timer != nullptr)
	{
		_close_timer->cancel(); 
		_close_timer = nullptr; 
	}
	
	_close_timer = std::make_shared<NetTimer>(GetIoContext());
	SetNetworkTimer(_close_timer, (int)NetworkTimer::Close, DEFAULT_NETWORK_CLOSE_TIMER_INTERVAL);
}

//====================================================================================================
// Set Timer
//====================================================================================================
void TcpNetworkObject::SetNetworkTimer(std::shared_ptr<NetTimer> network_timer, int id, int interval)
{
	if(_is_closeing == true)
	{
		return;
	}

	network_timer->expires_from_now(std::chrono::nanoseconds(interval*1000000));

	network_timer->async_wait(boost::bind(&TcpNetworkObject::OnNetworkTimer, 
		shared_from_this(), 
		boost::asio::placeholders::error, 
		network_timer, 
		id, 
		interval));
}

//====================================================================================================
// Timer Proc
//====================================================================================================
void TcpNetworkObject::OnNetworkTimer(const boost::system::error_code& error, 
									std::shared_ptr<NetTimer> network_timer, 
									int id, 
									int interval)
{	
	if(_is_closeing == true)
	{
		if(id == (int)NetworkTimer::PostClose)
		{
			if(PostCloseTimerProc() == false)
			{
				SetNetworkTimer(network_timer, id, interval);	
			}
		}
		
		return; 
	}
		
	if(error)
	{
		return; 
	}	

	switch(id)
	{
	case (int)NetworkTimer::KeepaliveSend :
		{
			if(_keepalive_send_callback != nullptr && (this->*_keepalive_send_callback)() == false)
			{
				return; 
			}			
		
			SetNetworkTimer(network_timer, id, interval);	
			
			break; 
		}
		case (int)NetworkTimer::KeepaliveCheck:
		{
			if(_keepalive_check_callbak != nullptr)
			{
				if(_keepalive_check_callbak != nullptr && (this->*_keepalive_check_callbak)() == false)
				{
					return; 
				}
			}
			
			SetNetworkTimer(network_timer, id, interval);	
			
			break; 
		}
		case (int)NetworkTimer::Close:
		{
			CloseTimerProc();
			break;
		}
		
	}
	
	return; 
}

//====================================================================================================
// Close Timer Proc 
//====================================================================================================
bool TcpNetworkObject::CloseTimerProc() 
{
	if(_is_closeing == false)
	{
		_is_closeing = true; 
		if(_network_callback != nullptr)_network_callback->OnNetworkClose(_object_key, _index_key, _remote_ip, _remote_port);
	}	
	
	return true;
}

//====================================================================================================
// PostCloseTimerProc
//====================================================================================================
bool TcpNetworkObject::PostCloseTimerProc() 
{
	//LOG_WRITE(("[%s] - TcpNetworkObject::PostCloseTimerProc", _object_name.c_str()));

	// 수신 데이터 처리 상태
	//if(m_bRecvDataProcessing == true)
	//{
	//	LOG_WARNING_WRITE(("[%s] TcpNetworkObject::PostCloseTimerProc - RecvData Processing", _object_name.c_str()));
	//	return false; 
	//}

	if(_keep_alive_send_timer != nullptr)	_keep_alive_send_timer->cancel(); 	// KeepAlive 전송 타이머 종료 
	if(_keepalive_check_timer != nullptr)	_keepalive_check_timer->cancel(); 	// KeepAlive 체크 타이머 종료 
 	if(_close_timer != nullptr)			_close_timer->cancel(); 			//종료 타이머 종료 
	if(_post_close_timer != nullptr)		_post_close_timer->cancel(); 		//Post 종료 타이머 종료 

	SocketClose();
	
	return true;
}

//====================================================================================================
// Send/Recv Traffic Rate
// - bps
//====================================================================================================
bool TcpNetworkObject::GetTrafficRate(double &send_bitrate, double &recv_bitrate)
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