//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "WebSocketObject.h"
#include <string.h>
#include <sha1.h>

#define HTTP_REQUEST_DATA_MAX_SIZE			(8192)
#define HTTP_HEADER_END_DELIMITER   		("\r\n\r\n")

static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

static inline bool is_base64(unsigned char c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len)
{
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--)
	{
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;

			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i < 4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while ((i++ < 3))
			ret += '=';

	}

	return ret;

}

std::string base64_decode(std::string const& encoded_string)
{
	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
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
// Http Header Date Gmt 시간 문자 
//====================================================================================================
std::string  CWebSocketNetworkObject::GetHttpHeaderDateTime()
{
	time_t		nTime = time(nullptr);
	struct tm	TimeStruct;
	char		TimeBuffer[100] = { 0, };

	TimeStruct = *gmtime(&nTime);

	strftime(TimeBuffer, sizeof(TimeBuffer), "%a, %d %b %Y %H:%M:%S GMT", &TimeStruct);

	return std::string(TimeBuffer);
}


//====================================================================================================
// 생성자  
//====================================================================================================
CWebSocketNetworkObject::CWebSocketNetworkObject()
{
	_is_handshake = false;
	service_name = "WebSocket";
}

//====================================================================================================
// 소멸자   
//====================================================================================================
CWebSocketNetworkObject::~CWebSocketNetworkObject()
{

}



//====================================================================================================
//  패킷 수신 
//====================================================================================================
int CWebSocketNetworkObject::RecvHandler(char* data, int data_size)
{
	int nProcessSize = 0;
	int read_size = 0;

	//파라메터 검증 
	if (data == nullptr || data_size <= 0)
	{
		SendErrorResponse("Bad Request");
		LOG_WRITE(("ERROR : [%s] RecvHandler - Param Fail - IndexKey(%d) IP(%s) Size(%d)", _object_name, _index_key, _remote_ip_string, data_size));
		return -1;
	}

	if (_is_handshake == false)
	{
		return RecvHandshakeProc((unsigned char*)data, data_size);

	}
	else
	{
		while (nProcessSize < data_size)
		{
			read_size = RecvDataProc((unsigned char*)(data + nProcessSize), data_size - nProcessSize);

			if (read_size == 0)
			{
				break;
			}
			else if (read_size < 0)
			{
				return read_size;
			}

			nProcessSize += read_size;
		}
	}

	return nProcessSize;


}

//====================================================================================================
//  Handshake 패킷 수신 처리 
//====================================================================================================
int CWebSocketNetworkObject::RecvHandshakeProc(unsigned char* data, int data_size)
{
	std::string::size_type	header_end_pos = 0;
	char* data_buffer = nullptr;
	std::string string_data;

	if (data_size > HTTP_REQUEST_DATA_MAX_SIZE)
	{
		SendErrorResponse("Bad Request");
		LOG_WRITE(("ERROR : [%s] RecvHandler - Data Size Over - IndexKey(%d) IP(%s) Size(%d)", _object_name, _index_key, _remote_ip_string, data_size));
		return -1;
	}

	data_buffer = new char[data_size + 1];
	memcpy(data_buffer, data, data_size);
	data_buffer[data_size] = 0;

	string_data = data_buffer;

	// Http 데이터 수신 확인(0 - 장상 처리)
	if (string_data.find(HTTP_HEADER_END_DELIMITER, header_end_pos) == std::string::npos)
	{
		LOG_WRITE(("WARNING : [%s] RecvHandler - Http Header End Fail - IndexKey(%d) IP(%s) Size(%d)", _object_name, _index_key, _remote_ip_string, data_size));
		return 0;
	}

	// 핸드 쉐이크 처리 
	if (DoHandshake(data_buffer) == false)
	{
		if (data_buffer != nullptr)
		{
			delete[] data_buffer;
			data_buffer = nullptr;
		}

		SendErrorResponse("Handshake Fail");
		LOG_WRITE(("ERROR : [%s] RecvHandler - DoHandshake Fail - IndexKey(%d) IP(%s)", _object_name, _index_key, _remote_ip_string));
		return -1;
	}

	if (data_buffer != nullptr)
	{
		delete[] data_buffer;
		data_buffer = nullptr;
	}

	_is_handshake = true;

	return data_size;

}

//====================================================================================================
//  데이터  패킷 수신 처리 
//====================================================================================================
int CWebSocketNetworkObject::RecvDataProc(unsigned char* data, int data_size)
{
	int 			payload_header_size = 2;
	long 			payload_size = 0;
	int				read_size = 0;
	bool 			mask = false;
	unsigned char* message = nullptr;
	int 			op_code = 0;
	char 			send_message[1024] = { 0, };
	int 			send_message_size = 0;

	if (data_size < WEBSOCKET_MIN_SIZE || data_size > WEBSOCKET_MAX_DATA_SIZE)
	{
		LOG_WRITE(("WARNING : [%s] RecvHandler - Websocket Min Size Fail - IndexKey(%d) IP(%s) Size(%d)", _object_name, _index_key, _remote_ip_string, data_size));
		return 0;
	}

	payload_size = (long)(data[1] & 0x7f);
	mask = ((data[1] & 0x80) == 0x80);

	if (payload_size == 126 && data_size < 4)
	{
		LOG_WRITE(("WARNING : [%s] RecvHandler - Websocket Min Size Fail 1 - IndexKey(%d) IP(%s) Size(%d)", _object_name, _index_key, _remote_ip_string, data_size));
		return 0;
	}

	if (payload_size == 127 && data_size < 10)
	{
		LOG_WRITE(("WARNING : [%s] RecvHandler - Websocket Min Size Fail 2 - IndexKey(%d) IP(%s) Size(%d)", _object_name, _index_key, _remote_ip_string, data_size));
		return 0;
	}

	if (payload_size == 126)
	{
		payload_size = (int)data[2] * 256 + (int)data[3];
		payload_header_size = 4;
	}
	else if (payload_size == 127)
	{
		BYTE* pSizeData = (BYTE*)& data[2];
		long 	header_size = 0;
		int 	nSize = 1;

		for (int nIndex = 7; nIndex >= 0; nIndex--)
		{
			header_size += (int)pSizeData[nIndex] * nSize;
			nSize *= 256;
		}

		payload_size = header_size;
		payload_header_size = 10;
	}

	if (mask == true)	read_size = payload_header_size + WEBSOCKET_MASKING_SIZE + payload_size;
	else				read_size = payload_header_size + payload_size;

	if (data_size < read_size || WEBSOCKET_MAX_DATA_SIZE < read_size)
	{
		LOG_WRITE(("WARNING : [%s] RecvHandler - Websocket Min Size Fail 3 - IndexKey(%d) IP(%s) Size(%d:%d)", _object_name, _index_key, _remote_ip_string, data_size, read_size));
		return 0;
	}


	message = new unsigned char[payload_size + 1];
	memset(message, 0, payload_size + 1);

	if (ParsingData((unsigned char*)data, read_size, message, payload_size + 1, payload_header_size, payload_size, mask, op_code) == false)
	{
		if (message != nullptr)
		{
			delete[] message;
			message = nullptr;
		}

		LOG_WRITE(("ERROR : [%s] RecvHandler - ParsingData Fail - IndexKey(%d) IP(%s) Size(%d:%d)", _object_name, _index_key, _remote_ip_string, data_size, read_size));
		return -1;
	}


	// debug
	if (op_code != WEBSOCKET_PONG_FRAME_TYPE)
	{
		LOG_WRITE(("INFO : [%s] RecvHandler - ParsingData  - IndexKey(%d) IP(%s) Size(%d)\n OP(%d) Message(%s)", _object_name, _index_key, _remote_ip_string, payload_size, op_code, message));
	}

	if (op_code == WEBSOCKET_TEXT_FRAME_TYPE && strcmp((const char*)message, "PING") == 0)
	{
		send_message_size = MakePingData("ping", (unsigned char*)send_message, sizeof(send_message));

		auto send_data = std::make_shared<std::vector<uint8_t>>(send_message, send_message + send_message_size);
	
		// 전송 
		if (PostSend(send_data) == false)
		{
			if (message != nullptr)
			{
				delete[] message;
				message = nullptr;
			}


			return -1;
		}

	}
	else if (op_code == WEBSOCKET_TEXT_FRAME_TYPE && strcmp((const char*)message, "CLOSE") == 0)
	{
		send_message_size = MakeCloseData("close", (unsigned char*)send_message, sizeof(send_message));

		auto send_data = std::make_shared<std::vector<uint8_t>>(send_message, send_message + send_message_size);

		// 전송 
		if (PostSend(send_data) == false)
		{
			if (message != nullptr)
			{
				delete[] message;
				message = nullptr;
			}


			return -1;
		}

	}
	else if (op_code == WEBSOCKET_PONG_FRAME_TYPE)
	{
		// Keepalive 처리 
	}
	else
	{
		if (RecvWebsocketRequest((const char*)message, payload_size, op_code) == false)
		{
			return -1;
		}
	}

	if (message != nullptr)
	{
		delete[] message;
		message = nullptr;
	}

	return read_size;
}


/**
@brief	클라이언트가 보낸 핸드쉐이크 요청 메세지를 분석하여 서버의 응답 메세지를 만든다.
@param	message	    클라이언트로 부터 수신된 요청 메세지
@param	pServiceName	웹소켓 서버의 서비스 이름 (ws://127.0.0.1:8888/ServiceName  -> ServiceName)
@param  out_buffer      서버의 응답 메세지를 담을 버퍼
@param  buffer_size     서버의 응답 메세지를 담을 버퍼의 크기 (최소 in_message 버퍼 크기만큼 할당)
@return
	- 성공 : true (out_buffer에 담긴 메세지를 클라이언트에 send 해주면 됨)
	- 실패 : false (프로토콜 버전이 맞지 않은 경우)
*/
bool CWebSocketNetworkObject::DoHandshake(const char* message)
{
	char sned_message[256] = { 0, };
	char szHeaderPattern[128] = { 0, };

	sprintf_s(szHeaderPattern, "GET /%s HTTP/1.1", service_name.c_str());

	if (strnicmp(message, szHeaderPattern, strlen(szHeaderPattern)) != 0)
	{
		return false;
	}

	const char* magicKey = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	std::string 			strMessage = message;
	std::string::size_type	nPos = strMessage.find("Sec-WebSocket-Key: ");

	if (nPos == 0 || nPos == std::string::npos)
	{
		return false;
	}

	std::string sec_key = strMessage.substr(nPos + strlen("Sec-WebSocket-Key: "));
	nPos = sec_key.find("==");

	if (nPos == 0 || nPos == std::string::npos)
	{
		return false;
	}

	sec_key = sec_key.substr(0, nPos + 2);

	if (sec_key.empty() == true)
	{
		return false;
	}


	char hash[21];
	sha1_ctx sha1;
	sha1_begin(&sha1);
	sha1_hash((unsigned char*)sec_key.c_str(), sec_key.length(), &sha1);
	sha1_hash((unsigned char*)magicKey, strlen(magicKey), &sha1);
	sha1_end((unsigned char*)hash, &sha1);

	std::string encoded = base64_encode((const unsigned char*)hash, 20);
	//string encoded = base64_encode((const unsigned char *)hash, 16);

	char* handshakeFormat = "HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: WebSocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: %s\r\n"
		"Sec-WebSocket-Protocol: %s\r\n\r\n";

	sprintf_s(sned_message, sizeof(sned_message), handshakeFormat, encoded.c_str(), service_name.c_str());


	auto send_data = std::make_shared<std::vector<uint8_t>>(sned_message, sned_message + strlen(sned_message));

	return PostSend(send_data);

}

/**
@brief	클라이언트가 웹소켓 프로토콜에 맞게 보낸 메세지를 해석한다.
@param	data	클라이언트가 보낸 프로토콜 메세지
@param	origin_len	클라이언트가 보낸 프로토콜 메세지의 길이
@param	target_data	프로토콜을 분석하여 파싱한 디코딩 메세지
@param	target_len	target_data 버퍼의 크기 (origin_data 버퍼 크기정도로 할당)
@param	opcode	    디코딩 메세지의 구분 (enum opcode 참조)
@return
	- 성공 : 디코딩된 메세지의 길이
	- 실패 : 0
*/
bool CWebSocketNetworkObject::ParsingData(unsigned char* data, int data_size, unsigned char* out_buffer, int out_buffer_size, int payload_header_size, int payload_size, bool mask, int& op_code)
{
	bool fin = ((data[0] & 0x80) == 0x80);
	bool rsv1 = ((data[0] & 0x40) == 0x40);
	bool rsv2 = ((data[0] & 0x20) == 0x20);
	bool rsv3 = ((data[0] & 0x10) == 0x10);

	op_code = (unsigned int)(data[0] & 0x0f);

	// fin check
	if (fin && op_code == 0x08)
	{
		return false;
	}

	memcpy(out_buffer, (const void*)& data[data_size - payload_size], payload_size);

	if (mask)
	{
		BYTE* pMaskingKey = &data[payload_header_size];

		for (int nIndex = 0; nIndex < payload_size; nIndex++)
		{
			out_buffer[nIndex] = (BYTE)(out_buffer[nIndex] ^ pMaskingKey[nIndex % 4]);
		}
	}

	return true;
}


/**
@brief	클라이언트로 보내고자 하는 텍스트 데이타를 웹소켓 프로토콜에 맞게 인코딩 함.
@param	data	        보내고자 하는 텍스트 데이타
@param	output_buffer	웹소켓 프로토콜에 맞게 인코딩한 데이타를 넣을 버퍼
@param	output_buffer_size	    target_buffer의 버퍼 크기 (최소 data 길이의 3배 이상 할당)
@return
	- 성공 : 인코딩 된 데이타의 크기 (target_buffer에 담긴 메세지를 클라이언트에 send 해주면 됨)
	- 실패 :  0 (target_buffer의 버퍼 크기가 너무 작음)
*/
int CWebSocketNetworkObject::MakeTextData(const char* data, unsigned char* output_buffer, int output_buffer_size)
{
	int 			data_size = strlen(data);
	unsigned char 	header[128];
	int 			header_size = 0;

	if (data_size < 126)
	{
		header_size = 2;

		header[1] = data_size;
	}
	else if (data_size < 65536)
	{
		header_size = 4;

		header[1] = 126;
		header[2] = data_size / 256;
		header[3] = data_size % 256;
	}
	else
	{
		header_size = 10;

		header[1] = 127;

		int left = data_size;
		int unit = 256;

		for (int i = 9; i > 1; i--)
		{
			header[i] = left % unit;
			left = left / unit;

			if (left == 0)
				break;
		}
	}

	if (output_buffer_size < header_size + data_size)
	{

		return 0;
	}

	header[0] = 0x01 | 0x80;

	memcpy(output_buffer, header, header_size);
	memcpy(output_buffer + header_size, data, data_size);

	return header_size + data_size;

}

/**
@brief	클라이언트로 보내고자 하는 바이너리 데이타를 웹소켓 프로토콜에 맞게 인코딩 함.
@param	data	        보내고자 하는 바이너리 데이타
@param	output_buffer	웹소켓 프로토콜에 맞게 인코딩한 데이타를 넣을 버퍼
@param	target_len	    target_buffer의 버퍼 크기 (최소 data 버퍼의 2배 이상 할당)
@return
	- 성공 : 인코딩 된 데이타의 크기 (target_buffer에 담긴 메세지를 클라이언트에 send 해주면 됨)
	- 실패 :  0 (target_buffer의 버퍼 크기가 너무 작음)
*/
int CWebSocketNetworkObject::MakeBinaryData(const char* data, int data_size, unsigned char* output_buffer, int output_buffer_size)
{
	unsigned char 	header[128];
	int 			header_size = 0;

	if (data_size < 126)
	{
		header_size = 2;

		header[1] = data_size;
	}
	else if (data_size < 65536)
	{
		header_size = 4;

		header[1] = 126;
		header[2] = data_size / 256;
		header[3] = data_size % 256;
	}
	else
	{
		header_size = 10;

		header[1] = 127;

		int left = data_size;
		int unit = 256;

		for (int i = 9; i > 1; i--)
		{
			header[i] = left % unit;
			left = left / unit;

			if (left == 0)
				break;
		}
	}

	if (output_buffer_size < header_size + data_size)
	{
		return 0;
	}

	header[0] = 0x02 | 0x80;

	memcpy(output_buffer, header, header_size);
	memcpy(output_buffer + header_size, data, data_size);

	return header_size + data_size;
}

/**
@brief	클라이언트로 보내고자 하는 PING 데이타를 웹소켓 프로토콜에 맞게 인코딩 함.
@param	data	        보내고자 하는 PING 메세지
@param	target_buffer	웹소켓 프로토콜에 맞게 인코딩한 데이타를 넣을 버퍼
@param	output_buffer_size	    target_buffer의 버퍼 크기 (최소 data 버퍼의 2배 이상 할당)
@return
	- 성공 : 인코딩 된 데이타의 크기 (target_buffer에 담긴 메세지를 클라이언트에 send 해주면 됨)
	- 실패 :  0 (target_buffer의 버퍼 크기가 너무 작음)
*/
int CWebSocketNetworkObject::MakePingData(const char* data, unsigned char* output_buffer, int output_buffer_size)
{
	unsigned char 	header[128];
	int 			header_size = 0;
	int 			data_size = strlen(data);

	if (data_size < 126)
	{
		header_size = 2;
		header[1] = data_size;
	}
	else if (data_size < 65536)
	{
		header_size = 4;

		header[1] = 126;
		header[2] = data_size / 256;
		header[3] = data_size % 256;
	}
	else
	{
		header_size = 10;

		header[1] = 127;

		int left = data_size;
		int unit = 256;

		for (int i = 9; i > 1; i--)
		{
			header[i] = left % unit;
			left = left / unit;

			if (left == 0)
				break;
		}
	}

	if (output_buffer_size < header_size + data_size)
	{
		return 0;
	}

	header[0] = 0x09 | 0x80;

	memcpy(output_buffer, header, header_size);
	memcpy(output_buffer + header_size, data, data_size);

	return header_size + data_size;
}

/**
@brief	클라이언트로 보내고자 하는 CLOSE 데이타를 웹소켓 프로토콜에 맞게 인코딩 함.
@param	data	        보내고자 하는 CLOSE 메세지
@param	output_buffer	웹소켓 프로토콜에 맞게 인코딩한 데이타를 넣을 버퍼
@param	output_buffer_size	    target_buffer의 버퍼 크기 (최소 data 버퍼의 2배 이상 할당)
@return
	- 성공 : 인코딩 된 데이타의 크기 (target_buffer에 담긴 메세지를 클라이언트에 send 해주면 됨)
	- 실패 :  0 (target_buffer의 버퍼 크기가 너무 작음)
*/
int CWebSocketNetworkObject::MakeCloseData(const char* data, unsigned char* output_buffer, int output_buffer_size)
{
	unsigned char 	header[128];
	int 			header_size = 0;

	int data_size = strlen(data);

	if (data_size < 126)
	{
		header_size = 2;

		header[1] = data_size;
	}
	else if (data_size < 65536)
	{
		header_size = 4;

		header[1] = 126;
		header[2] = data_size / 256;
		header[3] = data_size % 256;
	}
	else
	{
		header_size = 10;

		header[1] = 127;

		int left = data_size;
		int unit = 256;

		for (int i = 9; i > 1; i--)
		{
			header[i] = left % unit;
			left = left / unit;

			if (left == 0)
				break;
		}
	}

	if (output_buffer_size < header_size + data_size)
	{
		return 0;
	}

	header[0] = 0x08 | 0x80;

	memcpy(output_buffer, header, header_size);
	memcpy(output_buffer + header_size, data, data_size);

	return header_size + data_size;
}


//====================================================================================================
// Error 응답 
//====================================================================================================
bool CWebSocketNetworkObject::SendErrorResponse(const char* error_string)
{
	char data[4096] = { 0, };
	char body[4096] = { 0, };
	std::string date_time = GetHttpHeaderDateTime().c_str();

	// HTTP 데이터 설정 
	sprintf(body, "<html>\n"\
				"<head>\n"\
				"<title>error</title>\n"\
				"</head>\n"\
				"<body>\n"\
				"<br>%s\n"\
				"</body>\n"\
				"</html>", error_string);

	// HTTP 설정 
	sprintf(data, "%s 400 Bad Request\r\n"\
				"Date: %s\r\n"\
				"Server: http server\r\n"\
				"Content-Type: text/html\r\n"\
				"Accept-Ranges: bytes\r\n"\
				"Content-Length: %d\r\n\r\n%s"
				, http_version.c_str()
				, date_time.c_str()
				, (int)strlen(body)
				, body);


	auto send_data = std::make_shared<std::vector<uint8_t>>(body, body + strlen(body));

	return PostSend(send_data);
}

