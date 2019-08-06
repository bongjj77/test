#pragma once
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/pool/detail/guard.hpp>
#include <boost/pool/detail/mutex.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/bind.hpp>
#include <vector>
#include <deque>
#include <mutex>
#include <boost/asio/ssl.hpp>
#include <openssl/ssl.h>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#define DEAUULT_SOCKET_BUFFER_SZIE						(8192)
#define DEFAULT_MAX_SEND_QUEUE_SIZE						(5000) // 
#define DEFAULT_MAX_SEND_PACKET_SIZE					(25600)//200K bps 
#define DEFAULT_MAX_WAIT_SEND_DATA_SIZE					(30*1024*1024)// 20M 
#define DEFAULT_SEND_TIME_OUT							(20)	// second 

#define NETWORK_TRAFFIC_CHECK_TIME_INTERVAL 			(30)	//milliseconds
#define DEFAULT_NETWORK_POST_CLOSE_TIMER_INTERVAL 		(1)		//milliseconds
#define DEFAULT_NETWORK_CLOSE_TIMER_INTERVAL			(50)	//milliseconds
#define DEFAULT_NETWORK_TIME_OUT_CHECK_TIMER_INTERVAL	(1000)	//milliseconds(전송 최대 대기 시간) 90 ->10

enum class NetworkTimer
{
	KeepaliveSend,
	KeepaliveCheck,
	TimeoutCheck,
	Close,
	PostClose,
};

//====================================================================================================
// Boost 관련 정의 
//====================================================================================================
typedef boost::system::error_code		NetErrorCode;
typedef boost::asio::ip::tcp::socket	NetTcpSocket;
typedef boost::asio::ip::address_v4		NetAddress_v4;
typedef boost::asio::ip::tcp::acceptor	NetAcceptor;
typedef boost::asio::ip::tcp::resolver	NetResolver;
typedef boost::asio::ip::tcp::endpoint	NetTcpEndPoint;
typedef boost::asio::steady_timer		NetTimer;
typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket>  NetSocketSSL;

typedef boost::asio::ip::udp::socket 	NetUdpSocket;
typedef boost::asio::ip::udp::endpoint	NetUdpEndPoint;

typedef boost::asio::io_context NetIoContext;

//====================================================================================================
// 데이터  정의 
//====================================================================================================
struct NetSendData
{
	time_t	sned_time = 0; 
	std::shared_ptr<std::vector<uint8_t>> data; 
}; 

enum class NetConnectedResult
{
	Success = 0,
	Fail,
};

//====================================================================================================
// 현재 시간 ms 단위 
//====================================================================================================
static int64_t GetCurrentMilliSecond()
{
	return  std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
	
	/*
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, ts&);

	return ((ts.tv_sec) + (ts.tv_nsec / 1000000));
	*/

}


//====================================================================================================
// Network Interface 
//====================================================================================================
class INetworkCallback
{
public:
	virtual bool OnTcpNetworkAccepted(int object_key, NetTcpSocket * & socket, uint32_t ip, int port) = 0;			// Accepted
	virtual bool OnTcpNetworkAcceptedSSL(int object_key, NetSocketSSL * & socket_ssl, uint32_t ip, int port) = 0;	// Accepted(SSL)
	 
	virtual bool OnTcpNetworkConnected(int object_key,
		NetConnectedResult result,
		std::shared_ptr<std::vector<uint8_t>> connected_param,
		NetTcpSocket* socket,
		unsigned ip,
		int port) = 0;
	
	virtual bool OnTcpNetworkConnectedSSL(int object_key,
		NetConnectedResult result,
		std::shared_ptr<std::vector<uint8_t>> connected_param,
		NetSocketSSL* socket,
		unsigned ip,
		int port) = 0;


	virtual int OnNetworkClose(int object_key, int index_key, uint32_t ip, int port) = 0;

};
