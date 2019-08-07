#pragma once

#pragma pack(1) 

// Protocol Code Type
enum class PacketType : int32_t
{
	// Echo 요청/응답
	EchoRequest = 0,
	EchoResponse,

	// 특정 크기의 데이터 연속 전송 요청/응답/종료 
	StreamRequest = 100,
	StreamResponse,
	StreamEnd,
};

// Protocol Header
struct PacketHeader
{
	PacketType	type_code;
	int			data_size;
	uint8_t		data[0];
};
#pragma pack() 
