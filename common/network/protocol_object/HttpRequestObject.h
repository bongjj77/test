#pragma once
#include "../engine/tcp_network_object.h"
//====================================================================================================
// HttpRequestObject
//====================================================================================================
class HttpRequestObject : public TcpNetworkObject
{
public:
	HttpRequestObject();
	virtual ~HttpRequestObject();

public:
	bool IsComplete() { return _is_complete; }
	bool SendRequest(char* request_url, char* host);
	bool SendPostMethodRequest(char* request_url, const char* data, const char* host);

protected:
	virtual int RecvHandler(std::shared_ptr<std::vector<uint8_t>>& data);
	virtual void RecvContent(std::string& content) = 0;

private:
	bool _is_complete;
};
