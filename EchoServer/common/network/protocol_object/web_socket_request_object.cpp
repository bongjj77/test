#include "stdafx.h"
#include "web_socket_request_object.h"
#include <string.h>
#include <sstream>

#include "openssl/sha.h"


static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


bool WebSocketRequestObject::IsBase64(uint8_t c)
{
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string WebSocketRequestObject::Base64_Encode(uint8_t const* bytes_to_encode, uint32_t in_len)
{
  std::string ret;
  int i = 0;
  int j = 0;
  uint8_t char_array_3[3];
  uint8_t char_array_4[4];

  while (in_len--) 
  {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;

      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }

  return ret;

}

std::string WebSocketRequestObject::Base64Decode(std::string const& encoded_string)
{
	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	uint8_t char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && (encoded_string[in_] != '=') && IsBase64(encoded_string[in_]))
	{
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i == 4)
		{
			for (i = 0; i < 4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (j = 0; j < 4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
	}

	return ret;
}

//====================================================================================================
// 생성자  
//====================================================================================================
WebSocketRequestObject::WebSocketRequestObject()
{ 
	_handshake_complete		= false; 
	_web_socket_key = "x3JJHMbDL1EzLkh9GBhXDw==";
}

//====================================================================================================
// 소멸자   
//====================================================================================================
WebSocketRequestObject::~WebSocketRequestObject()
{
	
} 

//====================================================================================================
//  패킷 수신 
//====================================================================================================
int WebSocketRequestObject::RecvHandler(std::shared_ptr<std::vector<uint8_t>> &data)
{
	int process_size = 0;
	int read_size = 0; 
	
	//파라메터 검증 
	if(data == nullptr || data->size() <= 0)
	{
		LOG_WRITE(("ERROR : [%s] RecvHandler - Param Fail - IndexKey(%d) IP(%s) Size(%d)", 
					_object_name.c_str(), 
					_index_key, 
					_remote_ip_string.c_str(), 
					data->size()));
		return -1; 
	}
	
	// Handshake(Http header)
	if(!_handshake_complete)
	{
		read_size = RecvHandshakeProc(data);
		
		if (read_size <= 0 || read_size == data->size())
		{
			return read_size;
		}

		process_size += read_size;

	}
	
	while(process_size < data->size())
	{
		read_size = RecvDataProc((uint8_t *)(data->data() + process_size), data->size() - process_size);
			
		if(read_size == 0)
		{  
			break;
		}
		else if(read_size < 0)
		{
			return read_size;
		}
			
		process_size += read_size; 
	}
	
	return process_size; 
}

//====================================================================================================
//  Handshake 패킷 수신 처리 
// - http헤더 확인 
//====================================================================================================
int WebSocketRequestObject::RecvHandshakeProc(std::shared_ptr<std::vector<uint8_t>> &data)
{
	std::string::size_type 	header_end_pos = 0;

	std::string data_string(data->begin(), data->end());
	data_string.push_back(0);

	// Http 데이터 수신 확인(0 - 장상 처리)
	header_end_pos = data_string.find(HTTP_HEADER_END_DELIMITER); 
	if(header_end_pos == std::string::npos)
	{
		LOG_WRITE(("WARNING : [%s] RecvHandler - Http Header End Fail - IndexKey(%d) IP(%s) Size(%d)", 
					_object_name.c_str(), 
					_index_key, 
					_remote_ip_string.c_str(), 
					data->size()));

		return 0;
	}

	// header pattern 확인
	if (data_string.find("HTTP/1.1 101 Switching Protocols") == std::string::npos)
	{
		LOG_WRITE(("WARNING : [%s] RecvHandler - Http Header Pattern Fail - IndexKey(%d) IP(%s) Size(%d)",
					_object_name.c_str(),
					_index_key,
					_remote_ip_string.c_str(),
					data->size()));

		OnHandshakeComplete(false);

		return -1;
	}
	
	_handshake_complete = true;

	OnHandshakeComplete(true); 

	return header_end_pos + HTTP_HEADER_END_DELIMITER_SIZE;
			
}

//====================================================================================================
//  데이터  패킷 수신 처리 
//====================================================================================================
int WebSocketRequestObject::RecvDataProc(uint8_t * data, int data_size)
{
	int 		payload_header_size = 0;
	uint64_t	payload_size 		= 0;
	int			read_size			= 0; 
	bool 		is_mask				= false;
	int 		op_code				= 0;
		
	if(data_size < WEBSOCKET_MIN_SIZE)
	{
		/*
		LOG_WRITE(("WARNING : [%s] RecvDataProc - Websocket Min Size Fail - IndexKey(%d) IP(%s) Size(%d)", 
					_object_name.c_str(), 
					_index_key, 
					_remote_ip_string.c_str(), 
					data_size));
		*/
		return 0;
	}

	payload_size 	= (long)(data[1] & 0x7f);
	is_mask			= ((data[1] & 0x80) == 0x80);
	
	if((payload_size == 126 && data_size < 4) || (payload_size == 127 && data_size < 10))
	{
		/*
		LOG_WRITE(("WARNING : [%s] RecvDataProc - Websocket Min Size Fail 1 - IndexKey(%d) IP(%s) Size(%d)", 
					_object_name.c_str(), 
					_index_key, 
					_remote_ip_string.c_str(), 
					data_size));
		*/

		return 0;
	}
	
	if(payload_size == 126)
	{
		payload_size = (int)(data[2]<<8) + (int)(data[3]);
		payload_header_size = 4;
	}
	else if (payload_size == 127)
	{
		payload_size =	((uint64_t)data[2] << 56) +
						((uint64_t)data[3] << 48) +
						((uint64_t)data[4] << 40) +
						((uint64_t)data[5] << 32) +
						((uint64_t)data[6] << 24) +
						((uint64_t)data[7] << 16) +
						((uint64_t)data[8] << 8) +
						((uint64_t)data[9]);

		payload_header_size = 10;
	}

	if(is_mask == true)	read_size = payload_header_size + WEBSOCKET_MASKING_SIZE + payload_size;
	else				read_size = payload_header_size + payload_size;

	if(data_size < read_size)
	{
		/*
		LOG_WRITE(("WARNING : [%s] RecvHandler - Websocket Min Size Fail 3 - IndexKey(%d) IP(%s) Size(%d:%d)", 
					_object_name.c_str(), 
					_index_key, 
					_remote_ip_string.c_str(), 
					data_size, 
					read_size));
		*/

		return 0;
	}

	auto payload = std::make_shared<std::vector<uint8_t>>();
	
	if(!ParsingData(data, read_size, payload_header_size, payload_size, is_mask, op_code, payload))
	{
		LOG_WRITE(("ERROR : [%s] RecvHandler - ParsingData Fail - IndexKey(%d) IP(%s) Size(%d:%d)", 
					_object_name.c_str(), 
					_index_key, 
					_remote_ip_string.c_str(), 
					data_size, 
					read_size));

		return -1;
	}
	
	if(OnRecvWebsocketData(payload, (WebsocketOpcode)op_code) == false)
	{
		return -1;
	}

	return read_size;
}

//====================================================================================================
//  데이터  파싱
//  header(2~10Byte) + masking(4byte : option) + payload
//====================================================================================================
bool WebSocketRequestObject::ParsingData(uint8_t* data,
							int data_size,
							int payload_header_size,
							int payload_size,
							bool is_mask,
							int &op_code,
							std::shared_ptr<std::vector<uint8_t>> &payload)
{
	bool fin	= ((data[0] & 0x80) == 0x80);
	//bool rsv1	= ((data[0] & 0x40) == 0x40);
	//bool rsv2	= ((data[0] & 0x20) == 0x20);
	//bool rsv3	= ((data[0] & 0x10) == 0x10);
	
	op_code = (uint32_t)(data[0] & 0x0f);
	
	// fin check
	if (fin && op_code == 0x08)
	{
		return false;
	}

	if(is_mask)
	{
		payload->insert(payload->end(), (data + payload_header_size + WEBSOCKET_MASKING_SIZE), (data + payload_header_size + WEBSOCKET_MASKING_SIZE) + payload_size);

		uint8_t *masking_key = &data[payload_header_size];

		for (int index = 0; index < payload_size; index++)
        {
			payload->at(index) = (payload->at(index) ^ masking_key[index % 4]);
        }
	}
	else
	{
		payload->insert(payload->end(), (data + payload_header_size), (data + payload_header_size) + payload_size);
	}

	// 종료 문자 추가
	if (op_code == (int)WebsocketOpcode::TextFrame)
	{
		payload->push_back(0);
	}

	return true;
}


//====================================================================================================
// Handshake 전송(Client) 
// - http 형식
//
//====================================================================================================
int WebSocketRequestObject::MakeTextData(std::string &data, std::shared_ptr<std::vector<uint8_t>> &out_data)
{
	uint64_t data_size = data.size(); 

	out_data->push_back(((uint8_t)WebsocketOpcode::TextFrame | 0x80));

	if (data_size < 126)
	{
		out_data->push_back(data_size);
	}
	else if (data_size < 65536)
	{
		out_data->push_back(126);
		out_data->push_back((uint8_t)(data_size >> 8));
		out_data->push_back((uint8_t)(data_size & 0xff));
	}
	else
	{
		out_data->push_back(127);
		out_data->push_back((uint8_t)(data_size >> 56));
		out_data->push_back((uint8_t)(data_size >> 48));
		out_data->push_back((uint8_t)(data_size >> 40));
		out_data->push_back((uint8_t)(data_size >> 32));
		out_data->push_back((uint8_t)(data_size >> 24));
		out_data->push_back((uint8_t)(data_size >> 16));
		out_data->push_back((uint8_t)(data_size >> 8));
		out_data->push_back((uint8_t)(data_size & 0xff));
	}

	out_data->insert(out_data->end(), data.begin(), data.end());

	return out_data->size();

}

//====================================================================================================
// Handshake 전송
//====================================================================================================
bool WebSocketRequestObject::SendHandshake(std::string &host, std::string request_page)
{
	std::ostringstream http_data;
 
	http_data
	<< "GET /" << request_page << " HTTP/1.1\r\n"
	<< "Host: " << host << "\r\n"
	<< "Upgrade: websocket\r\n"
	<< "User-Agent: Chrome\r\n"
	<< "Connection: Upgrade\r\n"
	<< "Sec-WebSocket-Key: "<< _web_socket_key << "\r\n"
	<< "Sec-WebSocket-Version: 13\r\n\r\n";

	// 전송 
	std::string send_data = http_data.str();
	return PostSend(std::make_shared<std::vector<uint8_t>>(send_data.begin(), send_data.end()));
}
