#pragma once
#include "../engine/tcp_network_object.h"
#include "http_define_header.h"
//====================================================================================================
// WebSocketRequestObject
//====================================================================================================
class WebSocketRequestObject : public TcpNetworkObject 
{
public :
	WebSocketRequestObject();
	virtual				~WebSocketRequestObject();

public:
	static bool			IsBase64(uint8_t c);
	static std::string	Base64_Encode(uint8_t const* bytes_to_encode, uint32_t in_len);
	static std::string	Base64Decode(std::string const& encoded_string);
	static std::string 	GetHttpHeaderDateTime(); 
	
	bool				SendHandshake(std::string &host, std::string request_page); 
	
	bool				ParsingData(uint8_t* data, 
									int data_size, 
									int payload_header_size, 
									int payload_size, 
									bool is_mask, 
									int &op_code,
									std::shared_ptr<std::vector<uint8_t>> &output_buffer);

	int 				MakeTextData(std::string &data, std::shared_ptr<std::vector<uint8_t>> &out_data);
	
protected :
	virtual int			RecvHandler(std::shared_ptr<std::vector<uint8_t>> &data);
	int					RecvHandshakeProc(std::shared_ptr<std::vector<uint8_t>> &data);
	int					RecvDataProc(uint8_t * data, int data_size); 
	
	virtual bool		OnRecvWebsocketData(std::shared_ptr<std::vector<uint8_t>> &payload, WebsocketOpcode op_code) = 0;//  
	virtual bool		OnHandshakeComplete(bool result) = 0;

protected :
	bool				_handshake_complete;
	std::string			_web_socket_key; 

};
