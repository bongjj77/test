//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "test_tcp_server_object.h"

//====================================================================================================
//  TestTcpServerObject
//====================================================================================================
TestTcpServerObject::TestTcpServerObject()
{
	
	
}

//====================================================================================================
//  ~TestTcpServerObject
//====================================================================================================
TestTcpServerObject::~TestTcpServerObject()
{
	Destroy();
}

//====================================================================================================
//  Destroy
//====================================================================================================
void TestTcpServerObject::Destroy()
{
	
}

//====================================================================================================
// Create
//====================================================================================================
bool TestTcpServerObject::Create(TcpNetworkObjectParam *param)
{
	if(TcpNetworkObject::Create(param) == false)
	{
		return false;
	}
	
	return true;
}

//====================================================================================================
// 패킷 전송 
//====================================================================================================
bool TestTcpServerObject::SendPackt(PacketType type_code, int data_size, uint8_t *data)
{
	auto send_data = std::make_shared<std::vector<uint8_t>>(sizeof(PacketHeader) + data_size);
	PacketHeader* header = (PacketHeader*)send_data->data();

	header->type_code = type_code;
	header->data_size = data_size;
	memcpy(header->data, data, data_size);
	 
	return PostSend(send_data, false);
}

//====================================================================================================
// Echo 요청 패킷 
//====================================================================================================
bool TestTcpServerObject::SendEchoData(int data_size, uint8_t *data)
{
	return SendPackt(PacketType::EchoRequest, data_size, data);
}

//====================================================================================================
//  RecvHandler
//====================================================================================================
int TestTcpServerObject::RecvHandler(std::shared_ptr<std::vector<uint8_t>>& data)
{
	int				process_size = 0;
	const int		packet_header_size = sizeof(PacketHeader);
	PacketHeader* header = NULL;
	char* packet = NULL;

	// 패킷 처리  
	while (process_size < data->size())
	{
		header = NULL;
		packet = NULL;

		// Header 사이즈 수신 확인 
		if (data->size() < packet_header_size + process_size)
		{
			break;
		}

		// Header 설정 
		header = (PacketHeader*)(data->data() + process_size);

		// Header + Body 사이즈 수신 확인 
		if (data->size() < packet_header_size + header->data_size + process_size)
		{
			break;
		}

		RecvPacketProcess(header->type_code, header->data_size, (uint8_t*)header->data);

		// 처리 데이터 크기 설정 
		process_size += (packet_header_size + header->data_size);
	}

	return process_size;
}

//====================================================================================================
//  수신 패킷 처리 
//====================================================================================================
bool TestTcpServerObject::RecvPacketProcess(PacketType type_code, int data_size, uint8_t *data)
{
	//패킷 구분 
	switch (type_code)
	{
		case PacketType::EchoResponse: 		
			RecvEchoResponse(data_size, data); 	
			break;

		default:
		{
			LOG_WRITE(("ERROR : [%s] Unknown Packet - Type(%d) TestTcpServer(%s:%d)", _object_name.c_str(), type_code, _remote_ip_string.c_str(), _remote_port));
			break;
		}
	}

	return true;
}

//====================================================================================================
//   PacketType::EchoResponse Packet Process
//====================================================================================================
bool TestTcpServerObject::RecvEchoResponse(int data_size, uint8_t *data)
{
	LOG_WRITE(("INFO : [%s] Echo : %s ", _object_name.c_str(), data));
	return true;
}
