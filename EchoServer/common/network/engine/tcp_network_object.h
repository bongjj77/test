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
	void *object_callback;
	std::string object_name;

	NetTcpSocket *socket; 
	bool enable_ssl;
	NetSocketSSL *socket_ssl; 
	INetworkCallback *network_callback;
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
  	virtual bool	IsOpened();

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

protected :
	virtual void 	OnReceive(const  NetErrorCode & error, size_t data_size);
	void 			OnSend(const  NetErrorCode & error, size_t data_size);

	void			ClearSendDataQueue(); 
	
	void			SetNetworkTimer(NetTimer * network_timer, int id, int interval);
	void 			OnNetworkTimer(const boost::system::error_code& error, NetTimer * network_timer, int id, int interval);
	bool			CloseTimerProc();
	bool			PostCloseTimerProc();
	
	void			SetKeepAliveSendTimer(int interval, TcpKeepaliveSendCallback callback);
	void			SetKeepAliveCheckTimer(int interval, TcpKeepAliveCheckCallback callback);
 	void			SetCloseTimer();

	//상속 구현 클래스 
	virtual int		RecvHandler(std::shared_ptr<std::vector<uint8_t>> &data) = 0;  //수신 패킷 처리(필수) 

protected : 
	
	int					_object_key = -1; 
	int					_index_key = -1; 
	NetTcpSocket *		_socket = nullptr;
	INetworkCallback *	_network_callback = nullptr; 
	void *				_object_callback = nullptr; 

	bool				_is_support_ssl = false;
	NetSocketSSL * 		_socket_ssl = nullptr;	

	std::string			_remote_ip_string;
	uint32_t 			_remote_ip = 0; 
	int					_remote_port = 0; 
	std::string			_object_name = "unknown_tcp_object"; 
		
	std::deque<std::shared_ptr<NetSendData>> _send_datas;
	std::mutex			_send_data_queue_mutex;
	
	std::shared_ptr<std::vector<uint8_t>> _recv_buffer = nullptr;
	std::shared_ptr<std::vector<uint8_t>> _recv_data = nullptr;
	std::shared_ptr<std::vector<uint8_t>> _rest_data = nullptr;
		
	bool		_is_closeing = false;
	TcpKeepaliveSendCallback	_keepalive_send_callback; 
	TcpKeepAliveCheckCallback 	_keepalive_check_callbak; 
	uint32_t	_keepalive_check_time = 0; 
		
	NetTimer	*_keep_alive_send_timer = nullptr; 
	NetTimer	*_keepalive_check_timer = nullptr;
	NetTimer	*_timeount_check_timer = nullptr;
	NetTimer	*_close_timer = nullptr;
	NetTimer	*_post_close_timer = nullptr;

	time_t		_create_time = 0; 
	time_t		_last_send_complete_time = 0; 
	bool		_log_lock = false; 
	bool		_network_error = false; 
	bool		_is_send_completed_close = false; 
	
	int64_t		_traffic_check_time = 0;	// ms 
	uint64_t	_send_traffic = 0;			// bit 
	uint64_t	_recv_traffic = 0;			// bit
	
	int			_max_send_data_size = 0;
	int			_post_close_time_interval = 0; 	
};

