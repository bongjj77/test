#pragma once
#include "network_object_header.h"

#define UDP_NETWORK_BUFFER_SIZE			(8192)

class UdpNetworkObject;

//KeepAlive 콜백
typedef bool (UdpNetworkObject::*UdpKeepaliveSendCallback)(void);
typedef bool (UdpNetworkObject::*UdpKeepaliveCheckCallback)(void);




//====================================================================================================
// 생성 Param
//====================================================================================================
struct UdpNetworkObjectParam
{
	int 				object_key;
	uint32_t			remote_ip;
	int					remote_port;
	NetUdpSocket		*socket;
	INetworkCallback	*network_callback;
	void				*object_callback;
	std::string			object_name;
};


//====================================================================================================
// UdpNetworkObject
// - V1.0 
// - Client 기능만 구현
//====================================================================================================
class UdpNetworkObject : public std::enable_shared_from_this<UdpNetworkObject>, private boost::noncopyable
{
public :  
	explicit		UdpNetworkObject(bool is_send_data_lock = true);
	virtual 		~UdpNetworkObject();
	
public : 
	virtual bool	Create(UdpNetworkObjectParam *param);
	void			PostClose();
	void			Release();
	virtual bool	PostSend(std::shared_ptr<std::vector<uint8_t>> data, bool is_data_copy = false); 	// 외부 Thread 호출 가능 함수 주의

	void			SetIndexKey(int index_key) { _index_key = index_key; }
	int				GetIndexKey() { return _index_key; }
	uint32_t		GetRemoteIP() { return _remote_ip; }
	int				GetRemotePort() { return _remote_port; }
	std::string &	GetRemoteStringIP() { return _remote_ip_string; }
	
	// 네트워크 트래픽 관련 정보 
	bool			GetTrafficRate(double &send_bitrate, double &recv_bitrate);
	

	void			SetNetworkTimer(NetTimer * network_timer, int id, int interval);
	void 			OnNetworkTimer(const boost::system::error_code& error, NetTimer * network_timer, int id, int interval);
	bool			PostCloseTimerProc();

	void			SetKeepAliveSendTimer(int interval, UdpKeepaliveSendCallback callback);

protected :
	void			StartReceive(); 

	void 			OnSend(std::shared_ptr<std::vector<uint8_t>> const &data, const  NetErrorCode & error, size_t data_size);
	virtual void 	OnReceive(const  NetErrorCode & error, size_t data_size);
	
	//상속 구현 클래스 
	virtual void	RecvHandler(const std::shared_ptr<std::vector<uint8_t>> &data) = 0;	// 수신 패킷 처리(필수)
	virtual bool	OnHandshakeComplete(bool result) = 0;			// DTLS 핸드 쉐이크 완료

protected : 
	
	int				_index_key = -1;
	int				_object_key = -1;
	std::string		_object_name = "unknown_usp_object";
	INetworkCallback *	_network_callback = nullptr;
	void *			_object_callback = nullptr;

	std::string		_remote_ip_string;
	uint32_t 		_remote_ip = 0;
	int				_remote_port = 0;
	
	bool			_is_create = false;
	NetUdpSocket	*_socket = nullptr;
	NetUdpEndPoint	*_remote_end_point = nullptr;
	
	std::shared_ptr<std::vector<uint8_t>> _recv_buffer;
	
	int64_t			_traffic_check_time = 0; // ms 
	uint64_t		_send_traffic = 0; // bit 
	uint64_t		_recv_traffic = 0; // bit


	NetTimer		*_keep_alive_send_timer = nullptr;
	UdpKeepaliveSendCallback _keepalive_send_callback;

	bool _is_closeing = false; 
	NetTimer	*_post_close_timer = nullptr;

 };

