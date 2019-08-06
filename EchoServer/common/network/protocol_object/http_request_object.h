#pragma once
#include "network/engine/tcp_network_object.h"
#include "http_define_header.h"
//====================================================================================================
// HttpRequestObject
//====================================================================================================
class HttpRequestObject : public TcpNetworkObject 
{
public :
							HttpRequestObject();
	virtual					~HttpRequestObject();								

public:
	bool					IsComplete(){ return _is_complete; } 
	bool					SendRequest(char * request_url, char * host); 
	bool					SendPostMethodRequest(char * request_url, const char * data, const char * host); 

protected :
	virtual int				RecvHandler(char * data, int data_size);	
	virtual void			RecvContent(std::string & strContent) = 0;

private :
	bool					_is_complete; 
	
};
