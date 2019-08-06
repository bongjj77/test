#pragma once
#include "network/engine/tcp_network_object.h"
#include "http_define_header.h"
//====================================================================================================
// HttpResponseObject
//====================================================================================================
class HttpResponseObject : public TcpNetworkObject 
{
public :
						HttpResponseObject();
	virtual				~HttpResponseObject();								

public:
	bool				IsComplete(){ return _is_complete; } 
	virtual bool		SendResponse(std::string content_type, int data_size, char * data); 
	virtual bool		SendResponse(int header_size, char * header, int data_size, char * data); 
	virtual bool		SendErrorResponse(const char * error_string, int max_age); 
	virtual bool		SendErrorResponse(const char * error_string); 
	virtual bool		SendRedirectResponse(std::string content_type, std::string strRedirectURL, int data_size, char * data); 
	virtual bool		SendRedirectResponse(std::string strRedirectURL); 
	
	static std::string 	GetHttpHeaderDateTime(); 
	void				EnableCors(const char * pCorsOriginURL){ _cors_use = true; _cors_origin_url = pCorsOriginURL; } 
	void				SetCorsOriginList(const char * cors_origin_list){ _cors_origin_list = cors_origin_list; } 

	void				SetCookie(std::string strName, std::string strValue, std::string strDomain, std::string strPath); 
	
protected :
	virtual int			RecvHandler(std::shared_ptr<std::vector<uint8_t>> &data);
	virtual bool		RecvRequest(std::string & strRequestPage, std::string & strAgent) = 0;//strAgent값은 _agent_parsing == true에서 정상값 
	
protected :
	bool				_is_complete; 
	std::string			_http_version; 

	bool				_cors_use; 
	std::string			_cors_origin_url;
	std::string			_cors_origin_full_url;
	bool				_agent_parsing; 

	std::string			_cors_origin_list;

	std::string			_cookie;
	
	
	
};
