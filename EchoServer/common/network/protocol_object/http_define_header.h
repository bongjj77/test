#pragma once

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


#define HTTP_HEADER_END_DELIMITER   		("\r\n\r\n")
#define HTTP_HEADER_END_DELIMITER_SIZE 		(4)
#define HTTP_OK								("HTTP/1.1 200 OK")
#define HTTP_HEADER_FIELD_SEPERATOR			(":")
#define HTTP_LINE_END   					("\r\n")
#define HTTP_LINE_END_SIZE					(2)
#define HTTP_HEADER_CONTENT_LENGTH_FIELD	("Content-Length")


#define HTTP_REQUEST_DATA_MAX_SIZE			(8192)
#define HTTP_REQUEST_PAGE_INDEX				(1)
#define HTTP_VERSION_INDEX					(2)
#define HTTP_VERSION_1_0					("HTTP/1.0")
#define HTTP_VERSION_1_1					("HTTP/1.1")
#define HTTP_VERSION_MAX_SIZE				(50)


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

enum class WebsocketOpcode
{
    ContinuationFrame  = 0x0,
    TextFrame          = 0x1,
    BinaryFrame        = 0x2,
    CloseFrame         = 0x8,
    PingFrame          = 0x9,
    PongFrame          = 0xA
};
