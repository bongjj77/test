#pragma once
#include "../engine/tcp_network_object.h"

#define HTTP_ERROR_BAD_REQUEST 				(400)
#define HTTP_ERROR_NOT_FOUND				(404)
#define HTTP_TEXT_HTML_CONTENT_TYPE 		"text/html"
#define HTTP_TEXT_XML_CONTENT_TYPE 			"text/xml"
#define HTTP_IMAGE_JPG_CONTENT_TYPE 		"image/jpeg"
#define HTTP_IMAGE_GIF_CONTENT_TYPE 		"image/gif"
#define HTTP_APPLICATION_TEXT_CONTENT_TYPE 	"application/text"
#define HTTP_VIDEO_MP4_CONTENT_TYPE			"video/mp4"
#define HTTP_M3U8_CONTENT_TYPE				"application/vnd.apple.mpegURL"
#define HTTP_VIDE_MPEG_TS_CONTENT_TYPE		"video/MP2T"

/*
 0					 1					 2					 3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |	 Extended payload length	|
|I|S|S|S|  (4)	|A| 	(7) 	|			  (16/64)			|
|N|V|V|V|		|S| 			|	(if payload len==126/127)	|
| |1|2|3|		|K| 			|								|
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|	  Extended payload length continued, if payload len == 127	|
+ - - - - - - - - - - - - - - - +-------------------------------+
|								|Masking-key, if MASK set to 1	|
+-------------------------------+-------------------------------+
| Masking-key (continued)		|		   Payload Data 		|
+-------------------------------- - - - - - - - - - - - - - - - +
:					  Payload Data continued ...				:
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|					  Payload Data continued ...				|
+---------------------------------------------------------------+

*/


#define WEBSOCKET_MIN_SIZE 		(3)
#define WEBSOCKET_MASKING_SIZE	(4)
#define WEBSOCKET_MAX_DATA_SIZE	(65536+14)
#define WEBSOCKET_MAX_HEADER_SIZE (128)

typedef enum eWEBSOCKET_OPCODE_TYPE
{
	WEBSOCKET_CONTINUATION_FRAME_TYPE = 0x0,
	WEBSOCKET_TEXT_FRAME_TYPE = 0x1,
	WEBSOCKET_BINARY_FRAME_TYPE = 0x2,
	WEBSOCKET_CLOSE_FRAME_TYPE = 0x8,
	WEBSOCKET_PING_FRAME_TYPE = 0x9,
	WEBSOCKET_PONG_FRAME_TYPE = 0xA
}tWEBSOCKET_OPCODE_TYPE;


//====================================================================================================
// CWebSocketNetworkObject
//====================================================================================================
class CWebSocketNetworkObject : public TcpNetworkObject
{
public:
	CWebSocketNetworkObject();
	virtual~CWebSocketNetworkObject();

public:

	virtual bool SendErrorResponse(const char* error_string);
	static std::string 	GetHttpHeaderDateTime();

	bool DoHandshake(const char* message);
	bool ParsingData(unsigned char* data, int data_size, unsigned char* out_buffer, int out_buffer_size, int payload_header_size, int payload_size, bool mask, int& op_code);
	int MakeTextData(const char* data, unsigned char* output_buffer, int output_buffer_size);
	int MakeBinaryData(const char* data, int data_size, unsigned char* output_buffer, int output_buffer_size);
	int MakePingData(const char* data, unsigned char* output_buffer, int output_buffer_size);
	int MakeCloseData(const char* data, unsigned char* output_buffer, int output_buffer_size);

protected:
	virtual int RecvHandler(char* data, int data_size);
	int RecvHandshakeProc(unsigned char* data, int data_size);
	int RecvDataProc(unsigned char* data, int data_size);
	virtual bool RecvWebsocketRequest(const char* data, int data_size, int op_code) = 0;//  

protected:
	BOOL _is_handshake;
	std::string service_name;
	std::string http_version;
};
