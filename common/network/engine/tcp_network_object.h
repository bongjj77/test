//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include "network_object_header.h"

class TcpNetworkObject;

//KeepAlive 콜백
typedef bool (TcpNetworkObject::*TcpKeepaliveSendCallback)(void);
typedef bool (TcpNetworkObject::*TcpKeepAliveCheckCallback)(void);

//====================================================================================================
// Object Create Paam
//====================================================================================================
struct TcpNetworkObjectParam
{
	int object_key;
	std::shared_ptr<IObjectCallback> object_callback;
	std::string object_name;

	std::shared_ptr<NetTcpSocket> socket;	
	std::shared_ptr<ITcpNetwork> network_callback;
};

//====================================================================================================
// TcpNetworkObject(Session) 
//====================================================================================================
class TcpNetworkObject : public std::enable_shared_from_this<TcpNetworkObject>, private boost::noncopyable
{
public:
	explicit TcpNetworkObject();
	virtual ~TcpNetworkObject();

public:
	virtual bool	Create(TcpNetworkObjectParam *param); 
	virtual bool	Start();
	virtual bool	StartKeepAliveCheck(uint32_t nCheckTime){ _keepalive_check_time = nCheckTime; return true; } 

	void			SetRecvBufferSize(int buffer_size){ _recv_buffer->resize(buffer_size, 0); }
		
	void			SetIndexKey(int index_key){ _index_key = index_key; } 
	int				GetIndexKey(){ return _index_key; } 
  	
	uint32_t		GetRemoteIP(){ return _remote_ip; }
	int				GetRemotePort(){ return _remote_port; }
	std::string &	GetRemoteStringIP(){ return _remote_ip_string; }
	virtual time_t	GetCreateTime(){ return _create_time; }

	void			SetLogLock(bool bLogLock){ _log_lock = bLogLock; } 
	virtual bool	PostSend(std::shared_ptr<std::vector<uint8_t>> data, bool is_data_copy = false); 	 
	virtual void 	PostClose();  											 
	time_t			GetLastSendCompleteTime(){ return _last_send_complete_time; }

	void			SetPostCloseTimerInterval(int nMilliSecond){ _post_close_time_interval = nMilliSecond; } 
	bool			GetTrafficRate(double &send_bitrate, double &recv_bitrate);
	virtual bool	IsOpened();
protected :
	virtual void 	OnReceive(const  NetErrorCode & error, size_t data_size);
	void 			OnSend(const  NetErrorCode & error, size_t data_size);

	void			ClearSendDataQueue(); 
	
	void			SetNetworkTimer(std::shared_ptr<NetTimer> network_timer, int id, int interval);
	void 			OnNetworkTimer(const boost::system::error_code& error, std::shared_ptr<NetTimer> network_timer, int id, int interval);
	bool			CloseTimerProc();
	bool			PostCloseTimerProc();
	
	void			SetKeepAliveSendTimer(int interval, TcpKeepaliveSendCallback callback);
	void			SetKeepAliveCheckTimer(int interval, TcpKeepAliveCheckCallback callback);
 	void			SetCloseTimer();

	// interface 
	virtual int		RecvHandler(std::shared_ptr<std::vector<uint8_t>> &data) = 0;   

	virtual void	SocketClose();
	virtual boost::asio::io_context& GetIoContext();
	virtual void	AsyncRead();
	virtual void	AsyncWrite(std::shared_ptr<std::vector<uint8_t>> data);
  
protected : 
	
	int										_object_key = -1; 
	int										_index_key = -1; 
	std::shared_ptr<NetTcpSocket>			_socket = nullptr;
	std::shared_ptr<ITcpNetwork>		_network_callback = nullptr; 
	std::shared_ptr<IObjectCallback>		_object_callback = nullptr;
	
	std::string								_remote_ip_string;
	uint32_t 								_remote_ip = 0; 
	int										_remote_port = 0; 
	std::string								_object_name = "unknown_tcp_object"; 
		
	std::deque<std::shared_ptr<NetSendData>> _send_data_queue;
	std::mutex								_send_data_queue_mutex;
	
	std::shared_ptr<std::vector<uint8_t>>	_recv_buffer = nullptr;
	std::shared_ptr<std::vector<uint8_t>>	_recv_data = nullptr;
	std::shared_ptr<std::vector<uint8_t>>	_rest_data = nullptr;
		
	bool									_is_closeing = false;
	TcpKeepaliveSendCallback				_keepalive_send_callback; 
	TcpKeepAliveCheckCallback 				_keepalive_check_callbak; 
	uint32_t								_keepalive_check_time = 0; 
		
	std::shared_ptr<NetTimer>				_keep_alive_send_timer = nullptr; 
	std::shared_ptr<NetTimer>				_keepalive_check_timer = nullptr;
	std::shared_ptr<NetTimer>				_timeount_check_timer = nullptr;
	std::shared_ptr<NetTimer>				_close_timer = nullptr;
	std::shared_ptr<NetTimer>				_post_close_timer = nullptr;

	time_t									_create_time = 0; 
	time_t									_last_send_complete_time = 0; 
	bool									_log_lock = false; 
	bool									_network_error = false; 
	bool									_is_send_completed_close = false; 
	
	int64_t									_traffic_check_time = 0;	// ms 
	uint64_t								_send_traffic = 0;			// bit 
	uint64_t								_recv_traffic = 0;			// bit
	
	int										_max_send_data_size = 0;
	int										_post_close_time_interval = 0; 	
};

