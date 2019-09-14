//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "rtmp_chunk_stream.h"
#include "common/common_header.h"

//====================================================================================================
// Constructor 
//====================================================================================================
RtmpChunkStream::RtmpChunkStream()
{
	_stream_interface				= nullptr;
	_handshake_state				= RtmpHandshakeState::Ready;
	_import_chunk					= std::make_unique<RtmpImportChunk>(RTMP_DEFAULT_CHUNK_SIZE);
	_export_chunk					= std::make_unique<RtmpExportChunk>(false, RTMP_DEFAULT_CHUNK_SIZE);

	_media_info.video_timescale		= 1000;
	_media_info.audio_sampleindex	= 4;
	
	_stream_id						= 0;
	_peer_bandwidth					= RTMP_DEFAULT_PEER_BANDWIDTH;
	_acknowledgement_size			= RTMP_DEFAULT_ACKNOWNLEDGEMENT_SIZE/2;
	_acknowledgement_traffic		= 0; 
	_client_id						= 12345.0;
	_chunk_stream_id				= 0;		
	
	_video_sequence_load			= false;
	_audio_sequence_load			= false;
	_frame_config_info_load			= false;
}

//====================================================================================================
// Destructor 
//====================================================================================================
RtmpChunkStream::~RtmpChunkStream()
{
	Destroy(); 
}


//==============================================================================
// Create
//==============================================================================
bool RtmpChunkStream::Create(IRtmpChunkStream * stream_interface, int chunk_size/* = 4096*/)
{
	_stream_interface = stream_interface;
	
	return true;
}

//====================================================================================================
// Destroy
//====================================================================================================
void RtmpChunkStream::Destroy()
{
	
}

//====================================================================================================
// SendData
//====================================================================================================
bool RtmpChunkStream::SendData(uint32_t data_size, uint8_t * data)
{
	if(_stream_interface == nullptr)
	{
		return false;
	}

	if(_stream_interface->OnChunkStreamSend(data_size, (char *)data) == false)
	{
		return false;
	}

	return true;
}

//====================================================================================================
//  OnDataReceived
//====================================================================================================
int RtmpChunkStream::OnDataReceived(int data_size, char * data)
{
	int process_size = 0;

	auto process_data = std::make_shared<std::vector<uint8_t>>((uint8_t *)data, (uint8_t *)(data + data_size));;

	if (process_data->size() > MAX_MEDIA_PACKET_SIZE)
	{
		LOG_ERROR_WRITE(("RtmpChunkStream -  OnDataReceived - Process Size Fail - DataSize(%d) ProcessSize(%d)", data_size, process_size));
		return -1;
	}

	if (_handshake_state != RtmpHandshakeState::Complete)	process_size = ReceiveHandshakePacket(process_data);
	else													process_size = ReceiveChunkPacket(process_data);
	
	if (process_size < 0)
	{
		LOG_WRITE(("Process Size Fail - size(%d)", process_size));
		return -1;
	}
	 
	return process_size;
	
}

//====================================================================================================
// Handshak
//====================================================================================================
int RtmpChunkStream::ReceiveHandshakePacket(const std::shared_ptr<const std::vector<uint8_t>> &data)
{
	int32_t process_size = 0;
	int32_t chunk_process_size = 0;

	if (_handshake_state == RtmpHandshakeState::Ready)	process_size = (sizeof(uint8_t) + RTMP_HANDSHAKE_PACKET_SIZE);// c0 + c1
	else if (_handshake_state == RtmpHandshakeState::S2)process_size = (RTMP_HANDSHAKE_PACKET_SIZE);// c2
	else
	{
		LOG_WRITE(("Handshake State Fail - State(%d)", (int32_t)_handshake_state));
		return -1;
	}

	// Process Data Size Check 
	if (data->size() < process_size)
	{
		return 0;
	}

	// c0 + c1 / s0 + s1 + s2
	if (_handshake_state == RtmpHandshakeState::Ready)
	{
		if (data->at(0) != RTMP_HANDSHAKE_VERSION)
		{
			LOG_WRITE(("Handshake Version Fail - Version(%d:%d)", data->at(0), RTMP_HANDSHAKE_VERSION));
			return -1;
		}
		_handshake_state = RtmpHandshakeState::C0;

		if (!SendHandshake(data))
		{
			return -1;
		}

		return process_size;
	}

	_handshake_state = RtmpHandshakeState::C2;

	if (process_size < data->size())
	{
		auto  process_data = std::make_shared<std::vector<uint8_t>>(data->begin() + process_size, data->end());
		chunk_process_size = ReceiveChunkPacket(process_data);
		if (chunk_process_size < 0)
		{
			return -1;
		}
		process_size += chunk_process_size;
	}

	_handshake_state = RtmpHandshakeState::Complete;
	LOG_WRITE(("Handshake Complete"));
	return process_size;
}

//====================================================================================================
// SendHandshake 
// s0 + s1 + s2
//====================================================================================================
bool RtmpChunkStream::SendHandshake(const std::shared_ptr<const std::vector<uint8_t>> &data)
{
	uint8_t	s0 = 0;
	uint8_t	s1[RTMP_HANDSHAKE_PACKET_SIZE] = { 0, };
	uint8_t	s2[RTMP_HANDSHAKE_PACKET_SIZE] = { 0, };

	// ?곗씠???ㅼ젙
	s0 = RTMP_HANDSHAKE_VERSION;
	RtmpHandshake::MakeS1(s1);
	RtmpHandshake::MakeS2((uint8_t *)data->data() + sizeof(uint8_t), s2);
	_handshake_state = RtmpHandshakeState::C1;

	// s0 ?꾩넚
	if (!SendData(sizeof(s0), &s0))
	{
		LOG_WRITE(("Handshake s0 Send Fail"));
		return false;
	}
	_handshake_state = RtmpHandshakeState::S0;

	// s1 
	if (!SendData(sizeof(s1), s1))
	{
		LOG_WRITE(("Handshake s1 Send Fail"));
		return false;
	}

	_handshake_state = RtmpHandshakeState::S1;

	// s2
	if (!SendData(sizeof(s2), s2))
	{
		LOG_WRITE(("Handshake s2 Send Fail"));
		return false;
	}

	_handshake_state = RtmpHandshakeState::S2;

	return true;
}

//====================================================================================================
// ReceiveChunkPacket
//====================================================================================================
int32_t RtmpChunkStream::ReceiveChunkPacket(const std::shared_ptr<const std::vector<uint8_t>> &data)
{
	int32_t process_size = 0;
	int32_t	import_size = 0;
	bool	message_complete = false;

	while (process_size < data->size())
	{
		message_complete = false;

		import_size = _import_chunk->ImportStreamData((uint8_t *)data->data() + process_size, data->size() - process_size, message_complete);

		if (import_size == 0)
		{
			break;
		}
		else if (import_size < 0)
		{
			LOG_WRITE(("ImportStream Fail"));
			return import_size;
		}

		if (message_complete)
		{
			if (!ReceiveChunkMessage())
			{
				LOG_WRITE(("ReceiveChunkMessage Fail"));
				return -1;
			}
		}

		process_size += import_size;
	}

	//Acknowledgement append
	_acknowledgement_traffic += process_size;

	if (_acknowledgement_traffic > _acknowledgement_size)
	{
		SendAcknowledgementSize();

		// Init
		_acknowledgement_traffic = 0;
	}

	return process_size;
}

//====================================================================================================
// ReceiveChunkMessage
//====================================================================================================
bool RtmpChunkStream::ReceiveChunkMessage()
{
	while (true)
	{
		auto message = _import_chunk->GetMessage();

		if (message == nullptr || message->body == nullptr)
		{
			break;
		}

		if (message->message_header->body_size > MAX_MEDIA_PACKET_SIZE)
		{
			LOG_WRITE(("Packet Size Fail - Size(%u:%u)", message->message_header->body_size, MAX_MEDIA_PACKET_SIZE));
			return false;
		}

		bool bSuccess = true;

		//
		switch (message->message_header->type_id)
		{
			case RTMP_MSGID_AUDIO_MESSAGE:				bSuccess = ReceiveAudioMessage(message);	break; 
			case RTMP_MSGID_VIDEO_MESSAGE:				bSuccess = ReceiveVideoMessage(message);	break; 
			case RTMP_MSGID_SET_CHUNK_SIZE:				bSuccess = ReceiveSetChunkSize(message);	break; 
			case RTMP_MSGID_AMF0_DATA_MESSAGE:			ReceiveAmfDataMessage(message);				break;
			case RTMP_MSGID_AMF0_COMMAND_MESSAGE:		ReceiveAmfCommandMessage(message);			break;
			case RTMP_MSGID_WINDOWACKNOWLEDGEMENT_SIZE:	ReceiveWindowAcknowledgementSize(message);	break;
			default:
			{
				LOG_WRITE(("Unknown Type - Type(%d)", message->message_header->type_id));
				break;
			}
		}

		if (!bSuccess)
		{
			return false;
		}
	}

	return true;
}

//====================================================================================================
// Chunk Message - SetChunkSize
//====================================================================================================
bool RtmpChunkStream::ReceiveSetChunkSize(std::shared_ptr<ImportMessage> & message)
{
	auto chunk_size = RtmpMuxUtil::ReadInt32(message->body->data());

	if (chunk_size <= 0)
	{
		LOG_WRITE(("ChunkSize Fail - size(%d) ***", chunk_size));
		return false;
	}

	_import_chunk->SetChunkSize(chunk_size);
	LOG_WRITE(("Set Receive ChunkSize(%u)", chunk_size));

	return true;
}
//====================================================================================================
// Chunk Message - WindowAcknowledgementSize
//====================================================================================================
void RtmpChunkStream::ReceiveWindowAcknowledgementSize(std::shared_ptr<ImportMessage> & message)
{
	auto ackledgement_size = RtmpMuxUtil::ReadInt32(message->body->data());

	if (ackledgement_size != 0)
	{
		_acknowledgement_size = ackledgement_size / 2;
		_acknowledgement_traffic = 0;
	}
}

//====================================================================================================
// Chunk Message - Amf0CommandMessage
//====================================================================================================
void RtmpChunkStream::ReceiveAmfCommandMessage(std::shared_ptr<ImportMessage> & message)
{
	AmfDocument	document;
	std::string  message_name;
	double		transaction_id = 0.0;


	if (document.Decode(message->body->data(), message->message_header->body_size) == 0)
	{
		LOG_WRITE(("AmfDocument Size 0 "));
		return;
	}

	// Message Name  
	if (document.GetProperty(0) == nullptr || document.GetProperty(0)->GetType() != AmfDataType::String)
	{
		LOG_WRITE(("Message Name Fail"));
		return;
	}
	message_name = document.GetProperty(0)->GetString();


	// Message Transaction ID
	if (document.GetProperty(1) != nullptr && document.GetProperty(1)->GetType() == AmfDataType::Number)
	{
		transaction_id = document.GetProperty(1)->GetNumber();
	}

	//
	if		(message_name.compare(RTMP_CMD_NAME_CONNECT) == 0)		OnAmfConnect(message->message_header, document, transaction_id);
	else if (message_name.compare(RTMP_CMD_NAME_CREATESTREAM) == 0)	OnAmfCreateStream(message->message_header, document, transaction_id);
	else if (message_name.compare(RTMP_CMD_NAME_FCPUBLISH) == 0)	OnAmfFCPublish(message->message_header, document, transaction_id);
	else if (message_name.compare(RTMP_CMD_NAME_PUBLISH) == 0)		OnAmfPublish(message->message_header, document, transaction_id);
	else if (message_name.compare(RTMP_CMD_NAME_RELEASESTREAM) == 0){ ; }
	else if (message_name.compare(RTMP_PING) == 0)					{ ; }
	else if (message_name.compare(RTMP_CMD_NAME_DELETESTREAM) == 0)	OnAmfDeleteStream(message->message_header, document, transaction_id);
	else
	{
		LOG_WRITE(("Unknown Amf0CommandMessage - Message(%s:%.1f)", message_name.c_str(), transaction_id));
		return;
	}
}

//====================================================================================================
// Chunk Message - AMF0DataMessage 
//====================================================================================================
void RtmpChunkStream::ReceiveAmfDataMessage(std::shared_ptr<ImportMessage> & message)
{
	AmfDocument	document;
	int32_t		decode_lehgth = 0;
	std::string	message_name;
	std::string data_name;

	//
	decode_lehgth = document.Decode(message->body->data(), message->message_header->body_size);
	if (decode_lehgth == 0)
	{
		LOG_WRITE(("Amf0DataMessage Document Length 0"));
		return;
	}
	 
	if (document.GetProperty(0) != nullptr && document.GetProperty(0)->GetType() == AmfDataType::String)
	{
		message_name = document.GetProperty(0)->GetString();
	}
 
	if (document.GetProperty(1) != nullptr && document.GetProperty(1)->GetType() == AmfDataType::String)
	{
		data_name = document.GetProperty(1)->GetString();
	}
 
	if (message_name.compare(RTMP_CMD_DATA_SETDATAFRAME) == 0 && data_name.compare(RTMP_CMD_DATA_ONMETADATA) == 0 && document.GetProperty(2) != nullptr && (document.GetProperty(2)->GetType() == AmfDataType::Object || document.GetProperty(2)->GetType() == AmfDataType::Array))
	{
		OnAmfMetaData(message->message_header, document, 2);
	}
	else
	{
		LOG_WRITE(("Unknown Amf0DataMessage - Message(%s)", message_name.c_str()));
		return;
	}
}

//====================================================================================================
// OnAmfConnect
//====================================================================================================
void RtmpChunkStream::OnAmfConnect(std::shared_ptr<RtmpMuxMessageHeader> &message_header, AmfDocument &document, double transaction_id)
{
	double object_encoding = 0.0;

	if (document.GetProperty(2) != nullptr &&  document.GetProperty(2)->GetType() == AmfDataType::Object)
	{
		AmfObject	* 	object = document.GetProperty(2)->GetObject();
		int32_t			index;

		// object encoding  
		if ((index = object->FindName("objectEncoding")) >= 0 && object->GetType(index) == AmfDataType::Number)
		{
			object_encoding = object->GetNumber(index);
		}

		if ((index = object->FindName("app")) >= 0 && object->GetType(index) == AmfDataType::String)
		{
			_app_name = object->GetString(index);
			_stream_key.first = _app_name; 
		}
	}

	if (!SendWindowAcknowledgementSize())
	{
		LOG_WRITE(("SendWindowAcknowledgementSize Fail"));
		return;
	}

	if (!SendSetPeerBandwidth())
	{
		LOG_WRITE(("SendSetPeerBandwidth Fail"));
		return;
	}

	if (!SendStreamBegin())
	{
		LOG_WRITE(("SendStreamBegin Fail"));
		return;
	}

	if (!SendAmfConnectResult(message_header->chunk_stream_id, transaction_id, object_encoding))
	{
		LOG_WRITE(("SendAmfConnectResult Fail"));
		return;
	}

}

//====================================================================================================
// Amf Command - CreateStream
//====================================================================================================
void RtmpChunkStream::OnAmfCreateStream(std::shared_ptr<RtmpMuxMessageHeader> &message_header, AmfDocument &document, double transaction_id)
{
	if (!SendAmfCreateStreamResult(message_header->chunk_stream_id, transaction_id))
	{
		LOG_WRITE(("SendAmfCreateStreamResult Fail"));
		return;
	}

}

//====================================================================================================
// Amf Command - FCPublish
//====================================================================================================
void RtmpChunkStream::OnAmfFCPublish(std::shared_ptr<RtmpMuxMessageHeader> &message_header, AmfDocument &document, double transaction_id)
{
	if (_app_stream_name.empty() && document.GetProperty(3) != nullptr && document.GetProperty(3)->GetType() == AmfDataType::String)
	{
		if (!SendAmfOnFCPublish(message_header->chunk_stream_id, _stream_id, _client_id))
		{
			LOG_WRITE(("SendAmfOnFCPublish Fail"));
			return;
		}
		_app_stream_name = document.GetProperty(3)->GetString();
		_stream_key.second = _app_stream_name;
	}
}


//====================================================================================================
// Amf Command - Publish
//====================================================================================================
void RtmpChunkStream::OnAmfPublish(std::shared_ptr<RtmpMuxMessageHeader> &message_header, AmfDocument &document, double transaction_id)
{
	if (_app_stream_name.empty())
	{
		if (document.GetProperty(3) != nullptr &&  document.GetProperty(3)->GetType() == AmfDataType::String)
		{
			_app_stream_name = document.GetProperty(3)->GetString();
			_stream_key.second = _app_stream_name;
		}
		else
		{
			LOG_WRITE(("OnPublish - Publish Name None"));

			//Reject
			SendAmfOnStatus(message_header->chunk_stream_id, _stream_id, (char *)"error", (char *)"NetStream.Publish.Rejected", (char *)"Authentication Failed.", _client_id);
			return;
		}
	}

	_chunk_stream_id = message_header->chunk_stream_id;
	 
	if (((IRtmpChunkStream *)_stream_interface)->OnChunkStreamStart(_stream_key) == false)
	{
		LOG_ERROR_WRITE(("RtmpChunkStream - OnPublish - OnRtmpProviderStart"));

		//Reject 
		SendAmfOnStatus((uint32_t)_chunk_stream_id, _stream_id, (char *)"error", (char *)"NetStream.Publish.Rejected", (char *)"Authentication Failed.", _client_id);
		return;
	}
 
	if (!SendStreamBegin())
	{
		LOG_WRITE(("SendStreamBegin Fail"));
		return;
	}
 
	if (!SendAmfOnStatus((uint32_t)_chunk_stream_id, _stream_id, (char *)"status", (char *)"NetStream.Publish.Start", (char *)"Publishing", _client_id))
	{
		LOG_WRITE(("SendAmfOnStatus Fail"));
		return;
	}

}

//====================================================================================================
// Amf Command - Publish
//====================================================================================================
void RtmpChunkStream::OnAmfDeleteStream(std::shared_ptr<RtmpMuxMessageHeader> &message_header, AmfDocument &document, double transaction_id)
{
	LOG_WRITE(("Delete Stream - app(%s) stream(%s)", _app_name.c_str(), _app_stream_name.c_str()));

	//_delete_stream = true;
 	//_stream_interface->OnChunkStreamDelete(_remote, _app_name, _app_stream_name, _app_id, _app_stream_id);
}

//====================================================================================================
// SendMessagePacket
//====================================================================================================
bool RtmpChunkStream::SendMessagePacket(std::shared_ptr<RtmpMuxMessageHeader> &message_header, std::shared_ptr<std::vector<uint8_t>> &data)
{
	if (message_header == nullptr)
	{
		return false;
	}

	auto  export_data = _export_chunk->ExportStreamData(message_header, data);

	if (export_data == nullptr || export_data->data() == nullptr)
	{
		return false;
	}

	return SendData(export_data->size(), export_data->data());
}

//====================================================================================================
// SendAmfCommand
//====================================================================================================
bool RtmpChunkStream::SendAmfCommand(std::shared_ptr<RtmpMuxMessageHeader> &message_header, AmfDocument &document)
{
	auto     body = std::make_shared<std::vector<uint8_t>>(2048);
	uint32_t body_size = 0;

	if (message_header == nullptr)
	{
		return false;
	}

	// body
	body_size = (uint32_t)document.Encode(body->data());
	if (body_size == 0)
	{
		return false;
	}
	message_header->body_size = body_size;
	body->resize(body_size);

	return SendMessagePacket(message_header, body);
}

//====================================================================================================
// USendUserControlMessage
//====================================================================================================
bool RtmpChunkStream::SendUserControlMessage(uint16_t message, std::shared_ptr<std::vector<uint8_t>> &data)
{
	auto message_header = std::make_shared<RtmpMuxMessageHeader>(RTMP_CHUNK_STREAM_ID_URGENT, 0, RTMP_MSGID_USER_CONTROL_MESSAGE, 0, data->size() + sizeof(uint16_t));

	data->insert(data->begin(), 0);
	data->insert(data->begin(), 0);
	RtmpMuxUtil::WriteInt16(data->data(), message);

	message_header->body_size = data->size(); 

	return SendMessagePacket(message_header, data);
}

//====================================================================================================
// WindowAcknowledgementSize
//====================================================================================================
bool RtmpChunkStream::SendWindowAcknowledgementSize()
{
	auto body = std::make_shared<std::vector<uint8_t>>(sizeof(int));
	auto message_header = std::make_shared<RtmpMuxMessageHeader>(RTMP_CHUNK_STREAM_ID_URGENT, 0, RTMP_MSGID_WINDOWACKNOWLEDGEMENT_SIZE, _stream_id, body->size());

	RtmpMuxUtil::WriteInt32(body->data(), RTMP_DEFAULT_ACKNOWNLEDGEMENT_SIZE);

	return SendMessagePacket(message_header, body);
}

//====================================================================================================
// SendAcknowledgementSize
//====================================================================================================
bool RtmpChunkStream::SendAcknowledgementSize()
{
	auto body = std::make_shared<std::vector<uint8_t>>(sizeof(int));
	auto message_header = std::make_shared<RtmpMuxMessageHeader>(RTMP_CHUNK_STREAM_ID_URGENT, 0, RTMP_MSGID_ACKNOWLEDGEMENT, 0, body->size());

	RtmpMuxUtil::WriteInt32(body->data(), _acknowledgement_traffic);

	return SendMessagePacket(message_header, body);
}

//====================================================================================================
// SendSetPeerBandwidth
//====================================================================================================
bool RtmpChunkStream::SendSetPeerBandwidth()
{
	auto body = std::make_shared<std::vector<uint8_t>>(5);
	auto message_header = std::make_shared<RtmpMuxMessageHeader>(RTMP_CHUNK_STREAM_ID_URGENT, 0, RTMP_MSGID_SET_PEERBANDWIDTH, _stream_id, body->size());

	RtmpMuxUtil::WriteInt32(body->data(), _peer_bandwidth);
	RtmpMuxUtil::WriteInt8(body->data() + 4, 2);

	return SendMessagePacket(message_header, body);
}

//====================================================================================================
// SendStreamBegin
//====================================================================================================
bool RtmpChunkStream::SendStreamBegin()
{
	auto body = std::make_shared<std::vector<uint8_t>>(4);

	RtmpMuxUtil::WriteInt32(body->data(), _stream_id);

	return SendUserControlMessage(RTMP_UCMID_STREAMBEGIN, body);
}

//====================================================================================================
// SendAmfConnectResult
//====================================================================================================
bool RtmpChunkStream::SendAmfConnectResult(uint32_t chunk_stream_id, double transaction_id, double object_encoding)
{
	auto        message_header = std::make_shared<RtmpMuxMessageHeader>(chunk_stream_id, 0, RTMP_MSGID_AMF0_COMMAND_MESSAGE, _stream_id, 0);
	AmfDocument	document;
	AmfObject   *object = nullptr;
	AmfArray    *array = nullptr;

	// _result 
	document.AddProperty(RTMP_ACK_NAME_RESULT);
	document.AddProperty(transaction_id);

	// properties
	object = new AmfObject;
	object->AddProperty("fmsVer", "FMS/3,5,2,654");
	object->AddProperty("capabilities", 31.0);
	object->AddProperty("mode", 1.0);

	document.AddProperty(object);

	// information
	object = new AmfObject;
	object->AddProperty("level", "status");
	object->AddProperty("code", "NetConnection.Connect.Success");
	object->AddProperty("description", "Connection succeeded.");
	object->AddProperty("clientid", _client_id);
	object->AddProperty("objectEncoding", object_encoding);

	array = new AmfArray;
	array->AddProperty("version", "3,5,2,654");
	object->AddProperty("data", array);

	document.AddProperty(object);

	return SendAmfCommand(message_header, document);
}

//====================================================================================================
// SendAmfOnFCPublish
//====================================================================================================
bool RtmpChunkStream::SendAmfOnFCPublish(uint32_t chunk_stream_id, uint32_t stream_id, double client_id)
{
	auto message_header = std::make_shared<RtmpMuxMessageHeader>(chunk_stream_id, 0, RTMP_MSGID_AMF0_COMMAND_MESSAGE, _stream_id, 0);
	AmfDocument	document;
	AmfObject   *object = nullptr;

	document.AddProperty(RTMP_CMD_NAME_ONFCPUBLISH);
	document.AddProperty(0.0);
	document.AddProperty(AmfDataType::Null);

	object = new AmfObject;
	object->AddProperty("level", "status");
	object->AddProperty("code", "NetStream.Publish.Start");
	object->AddProperty("description", "FCPublish");
	object->AddProperty("clientid", client_id);

	document.AddProperty(object);

	return SendAmfCommand(message_header, document);
}


//====================================================================================================
// SendAmfCreateStreamResult
//====================================================================================================
bool RtmpChunkStream::SendAmfCreateStreamResult(uint32_t chunk_stream_id, double transaction_id)
{
	auto message_header = std::make_shared<RtmpMuxMessageHeader>(chunk_stream_id, 0, RTMP_MSGID_AMF0_COMMAND_MESSAGE, 0, 0);
	AmfDocument	document;
 
	_stream_id = 1;

	document.AddProperty(RTMP_ACK_NAME_RESULT);
	document.AddProperty(transaction_id);
	document.AddProperty(AmfDataType::Null);
	document.AddProperty((double)_stream_id);

	return SendAmfCommand(message_header, document);
}


//====================================================================================================
// SendAmfOnStatus
//====================================================================================================
bool RtmpChunkStream::SendAmfOnStatus(uint32_t chunk_stream_id, uint32_t stream_id, char * level, char * code, char * description, double client_id)
{
	auto	    message_header = std::make_shared<RtmpMuxMessageHeader>(chunk_stream_id, 0, RTMP_MSGID_AMF0_COMMAND_MESSAGE, stream_id, 0);
	AmfDocument	document;
	AmfObject   *object = nullptr;

	document.AddProperty(RTMP_CMD_NAME_ONSTATUS);
	document.AddProperty(0.0);
	document.AddProperty(AmfDataType::Null);

	object = new AmfObject;
	object->AddProperty("level", level);
	object->AddProperty("code", code);
	object->AddProperty("description", description);
	object->AddProperty("clientid", client_id);

	document.AddProperty(object);

	return SendAmfCommand(message_header, document);
}


//====================================================================================================
// ReceiveAudioMessage
//====================================================================================================
bool RtmpChunkStream::ReceiveAudioMessage(std::shared_ptr<ImportMessage> &message)
{
	int 				frame_size		= 0; 
	uint8_t				*data			= nullptr;
	uint8_t				*frame_data			= nullptr; 
	int 				data_size 		= 0; 
	uint8_t				control_header	= 0; 
	bool				sequence_data	= false; 
	CodecType	codec_type		= _media_info.audio_codec_type;

	std::shared_ptr<RtmpMuxMessageHeader>	header = message->message_header;
	uint8_t									*body = message->body->data();
											 
	if (!_media_info.video_streaming)
	{
		return true;
	}

	if(header->body_size > MAX_MEDIA_PACKET_SIZE || header->body_size <= 0)
	{
		LOG_ERROR_WRITE(("RtmpChunkStream - RecvAudioMessage - Header Size Fail - size(%d)", header->body_size));
		return false; 
	}	
	
 	data			= (uint8_t *)body; 
	data_size 		= header->body_size; 
	control_header	= data[RTMP_AUDIO_CONTROL_HEADER_INDEX]; 

	frame_data		= data + RTMP_AAC_AUDIO_FRAME_INDEX;
	frame_size		= data_size - RTMP_AAC_AUDIO_FRAME_INDEX;
	sequence_data  	= data[RTMP_AAC_AUDIO_SEQUENCE_HEADER_INDEX] == RTMP_SEQUENCE_INFO_TYPE ? true : false;
		

	if(sequence_data == true && data_size >= 4)
	{
		
		int sample_index 	= 0; 
		int samplerate 	= 0; 
		int nChannel		= 0;
			
		sample_index 	+= ((data[2] & 0x07)<<1);
		sample_index 	+= (data[3]>>7);
		if(sample_index < SAMPLERATE_TABLE_SIZE)
		{
			samplerate 	= g_sample_rate_table[sample_index];
		}
			
		nChannel  =  data[3]>>3 & 0x0f;
			
		LOG_WRITE(("*** INFO : RtmpChunkStream - RecvAudioMessage Sequence Parsing- SampleRate(%d) Channel(%d) ***", samplerate, nChannel));	
				
		_media_info.audio_samplerate 	= samplerate;
		_media_info.audio_sampleindex 	= sample_index;
		_media_info.audio_channels = nChannel;

		_audio_sequence_load = true;

		if (_frame_config_info_load == false)StreamInfoLoadCheck();

		return true; 			
	}
	
	if(_audio_sequence_load == false)
	{
		return true; 
	}

	auto frame = std::make_shared<FrameInfo>(header->timestamp, 0, RTMP_TIMESCALE,FrameType::AudioFrame, frame_size, frame_data);

	return ((IRtmpChunkStream *)_stream_interface)->OnChunkStreamData(_stream_key, frame);
}

//====================================================================================================
// ReceiveVideoMessage
//====================================================================================================
bool RtmpChunkStream::ReceiveVideoMessage(std::shared_ptr<ImportMessage> &message)
{
	uint8_t  	control_header 	= 0;
	int 		frame_size 		= 0; 
	uint8_t		*body_data		= nullptr;
	uint8_t		*frame_data		= nullptr; 
	FrameType 	frame_type		= FrameType::VideoPFrame; 
	int	   	 	skip_size		= 0; 
	int			body_size		= 0; 
	int			cts = 0;
	bool		sequence_data	= false; 
	int			padding_size	= 0; 
	char		*avc_frame 		= nullptr; 
	int	 		avc_frame_size	= 0; 

	std::shared_ptr<RtmpMuxMessageHeader>	header = message->message_header;
	uint8_t									*body = message->body->data();

	if (!_media_info.audio_streaming)
	{
		return true;
	}

	if(header->body_size <= RTMP_VIDEO_DATA_MIN_SIZE || header->body_size > MAX_MEDIA_PACKET_SIZE)
	{
		LOG_ERROR_WRITE(("RtmpChunkStream - RecvVideoMessage - Header Size Fail - size(%d)", header->body_size));
		return false; 
	}
		
	body_data			= (uint8_t *)body;
	body_size			= header->body_size;
	control_header		= body_data[RTMP_VIDEO_CONTROL_HEADER_INDEX];
	sequence_data  		= body_data[RTMP_VIDEO_SEQUENCE_HEADER_INDEX] == RTMP_SEQUENCE_INFO_TYPE ? true : false;

	if		(control_header == RTMP_H264_I_FRAME_TYPE)frame_type = FrameType::VideoKeyFrame;	//I-Frame
	else if	(control_header == RTMP_H264_P_FRAME_TYPE)frame_type = FrameType::VideoPFrame; 	//P-Frame
	else 
	{
		LOG_ERROR_WRITE(("RtmpChunkStream - RecvVideoMessage - Unknown Frame Type - Type(0x%x)", control_header));
		return false; 
	}
	 
	if(_video_sequence_load == false && sequence_data == true)
	{
		 
		std::unique_ptr<std::vector<uint8_t>> data = std::make_unique<std::vector<uint8_t>>(message->body->begin() + 2, message->body->end());

		//SPS/PPS Load 
		if(!ProcessVideoSequenceData(std::move(data)))
		{
			LOG_ERROR_WRITE(("RtmpChunkStream - RecvVideoMessage - LoadVideoConfig "));
			return true;
		}
 
		_video_sequence_load = true;
 
		if(_frame_config_info_load == false) StreamInfoLoadCheck();
	 
		return true; 
	}

	if(_video_sequence_load == false)
	{
		return true; 
	} 
 
	if(header->body_size < RTMP_VIDEO_FRAME_INDEX)
	{
		LOG_ERROR_WRITE(("RtmpChunkStream - RecvVideoMessage - Frame Length Fail - Length(%u:%d)", header->body_size, RTMP_VIDEO_FRAME_INDEX));
		return false; 
	}

	// + 9( [Control Header:1] + [type:1] + [Composition Offset:3] + [Size:4]) 
	cts				= RtmpMuxUtil::ReadInt24(body_data + RTMP_VIDEO_COMPOSITION_OFFSET_INDEX);
	frame_size		= RtmpMuxUtil::ReadInt32(body_data + RTMP_VIDEO_FRAME_SIZE_INDEX); 
	frame_data		= body_data + RTMP_VIDEO_FRAME_INDEX;
	padding_size 	= (RTMP_VIDEO_FRAME_SIZE_INFO_SIZE + frame_size); 

	if(frame_size <= 0 || frame_size > MAX_MEDIA_PACKET_SIZE || frame_size > (body_size - RTMP_VIDEO_FRAME_INDEX))
	{
		LOG_ERROR_WRITE(("RtmpChunkStream - RecvVideoMessage - Frame Size Fail - size(%d)", frame_size));
		return false;
	}
	
 
	while((RTMP_VIDEO_FRAME_SIZE_INDEX + padding_size)+ RTMP_VIDEO_FRAME_SIZE_INFO_SIZE < body_size)
	{
	 
		if( (RTMP_VIDEO_FRAME_SIZE_INDEX + padding_size) + RTMP_VIDEO_FRAME_SIZE_INFO_SIZE + 2 <= body_size && 
			frame_data[frame_size + RTMP_VIDEO_FRAME_SIZE_INFO_SIZE] == 0x0c &&
			(frame_data[frame_size + RTMP_VIDEO_FRAME_SIZE_INFO_SIZE+1] == 0xff || frame_data[frame_size + RTMP_VIDEO_FRAME_SIZE_INFO_SIZE+1] == 0x80))
		{
			break;
		}
						
		frame_data += frame_size;
		
		frame_size = RtmpMuxUtil::ReadInt32(frame_data);
 
		if(frame_size > MAX_MEDIA_PACKET_SIZE || frame_size <= 0 )
		{
			LOG_ERROR_WRITE((" RtmpChunkStream - RecvVideoMessage - Padding Frame Size Fail - size(%d)", frame_size));
			return false;
		}

		frame_data += RTMP_VIDEO_FRAME_SIZE_INFO_SIZE;
		padding_size += (RTMP_VIDEO_FRAME_SIZE_INFO_SIZE + frame_size);
	}
	 
	if(frame_size <= 0 || (RTMP_VIDEO_FRAME_SIZE_INDEX + padding_size) > body_size)
	{
		LOG_ERROR_WRITE(("RtmpChunkStream - RecvVideoMessage - Frame Size Fail - Size(%d:%d)", frame_size, body_size));
		return false; 
	}
	
	auto frame = std::make_shared<FrameInfo>(header->timestamp, cts, RTMP_TIMESCALE, frame_type, frame_size, frame_data);
	return ((IRtmpChunkStream *)_stream_interface)->OnChunkStreamData(_stream_key, frame);
}

//====================================================================================================
// Load Adt(AAC)
// - Audio Cntrol Byte(1Byte)
//	 Format(4Bit)	      - 10(AAC), 2(MP3), 11(SPEEX)
//   Sample rate(2Bit)	- 1(11khz), 2(22khz)/3(44Khz) 
//   Sample size(1bit)	- 1(16bit) 0 (8bit) 
//   Channels(1bit) 		- 1(stereo) 0 (mono)
//- 2Byte( AAAAABBB BCCCCDEF )
// A: 00010 - LC; 
// B: 1000 - sample rate index (8 --- 16000Hz)
// C: 0001 - 1 channel (front-center); 
// D: 0 - 1024 frame size; 
// E: 0 - depends on core; 
// F: 0 - ext. flag
//====================================================================================================
bool RtmpChunkStream::ProcessAudioSequenceData(std::unique_ptr<std::vector<uint8_t>> data)
{
	int sample_index = 0;
	int samplerate = 0;
	int channels = 0;

	// 理쒖냼  湲몄씠 2byte
	if (data->size() < 2)
	{
		LOG_WRITE(("Data Size Fail - size(%d)", data->size()));
		return false;
	}

	sample_index += (data->at(0) & 0x07) << 1;
	sample_index += data->at(1) >> 7;
	if (sample_index >= SAMPLERATE_TABLE_SIZE)
	{
		LOG_WRITE(("Sampleindex Fail - index(%d)", sample_index));
		return false;
	}

	samplerate = g_sample_rate_table[sample_index];
	channels = data->at(1) >> 3 & 0x0f;

	_media_info.audio_samplerate = samplerate;
	_media_info.audio_sampleindex = sample_index;
	_media_info.audio_channels = channels;

	LOG_WRITE(("Audio Sequence Data - samplerate(%d) channels(%d)", samplerate, channels));

	return true;
}


//====================================================================================================
// ProcessVideoSequenceData
// - SPS/PPS Load
//      - 00 00 00   		- 3 byte
//      - version 			- 1 byte
//      - profile			- 1 byte
//      - Compatibility		- 1 byte
//      - level				- 1 byte
//      - length size in byte 	- 1 byte
//      - number of sps		- 1 byte(0xe1/0x01)
//      - sps size			- 2 byte  *
//      - sps data			- n byte  *
//      - number of pps 	- 1 byte
//      - pps size			- 2 byte  *
//      - pps data			- n byte  *
//====================================================================================================
bool RtmpChunkStream::ProcessVideoSequenceData(std::unique_ptr<std::vector<uint8_t>> data)
{
	int sps_size = 0;
	int pps_size = 0;

	if (data->size() < RTMP_SPS_PPS_MIN_DATA_SIZE || data->at(0) != 0 || data->at(1) != 0)
	{
		LOG_WRITE(("Data Size Fail - size(%d)", data->size()));
		return false;
	}

	sps_size = RtmpMuxUtil::ReadInt16(data->data() + 9);
	if (sps_size <= 0 || sps_size > (data->size() - 11))
	{
		LOG_WRITE(("SPS Size Fail - sps(%d)", sps_size));
		return false;
	}

	pps_size = RtmpMuxUtil::ReadInt16(data->data() + 12 + sps_size);
	if (pps_size <= 0 || pps_size > (data->size() - 14 - sps_size))
	{
		LOG_WRITE(("PPS Size Fail - pps(%d:%d)", pps_size));
		return false;
	}

	_media_info.avc_sps->assign(data->begin() + 11, data->begin() + 11 + sps_size); // SPS
	_media_info.avc_pps->assign(data->begin() + 14 + sps_size, data->begin() + 14 + sps_size + pps_size);  // PPS
	
	return true;
}

//====================================================================================================
// StreamInfoLoadCheck 
//====================================================================================================
bool RtmpChunkStream::StreamInfoLoadCheck()
{
	if (_media_info.video_streaming == true && _video_sequence_load == false)
	{
		return false;
	}
	else if (_media_info.audio_streaming == true && _audio_sequence_load == false)
	{
		return false;
	}

	_frame_config_info_load = true;

	return ((IRtmpChunkStream *)_stream_interface)->OnChunkStreamReadyComplete(_stream_key, _media_info);
}


//====================================================================================================
// AMF Command - OnMetaData
//====================================================================================================
bool RtmpChunkStream::OnAmfMetaData(std::shared_ptr<RtmpMuxMessageHeader> &message_header, AmfDocument &document, int32_t object_index)
{
	CodecType	video_codec_type 	= CodecType::H264;
	CodecType	audio_codec_type	= CodecType::AAC;
	double		frame_rate 			= 30.0; 
	double		video_width			= 0; 
	double		video_height		= 0; 
	double		video_bitrate		= 0; 
	double		audio_bitrate		= 0.0; 
	double		audio_channels		= 1.0; 
	double		audio_samplerate	= 0.0;
	double		audio_samplesize	= 0.0;
	//double 	audio_samplerate	= 0.0; 
	AmfObjectArray * object			= nullptr; 
	int			index 				= 0;
	std::string bitrate_string;
	std::string device_type_string	= RTMP_UNKNOWN_DEVICE_TYPE_STRING; 
	std::string device_guid_string	; 
	EncoderType encoder_type		= EncoderType::Custom;
		 
	if(document.GetProperty(object_index)->GetType() == AmfDataType::Object)	
		object = (AmfObjectArray *)(document.GetProperty(object_index)->GetObject());
	else 																
		object = (AmfObjectArray *)(document.GetProperty(object_index)->GetArray());
	
	// DeviceType
	if ((index = object->FindName("videodevice")) >= 0 && object->GetType(index) == AmfDataType::String)	
		device_type_string =  object->GetString(index); 	//DeviceType - XSplit
	else if	((index = object->FindName("encoder")) >= 0 && object->GetType(index) == AmfDataType::String)		
		device_type_string =  object->GetString(index);	//DeviceType - OBS
																																					// Encoder ?몄떇
	if		(strstr(device_type_string.c_str(),"Open Broadcaster") >= 0)	encoder_type = EncoderType::OBS;
	else if (strstr(device_type_string.c_str(), "obs-output") >= 0)			encoder_type = EncoderType::OBS;
	else if (strstr(device_type_string.c_str(), "XSplitBroadcaster") >= 0)	encoder_type = EncoderType::Xsplit;
	else if (strstr(device_type_string.c_str(), "Lavf") >= 0)				encoder_type = EncoderType::Lavf;
	else																	encoder_type = EncoderType::Custom;
 
	if((index = object->FindName("videocodecid")) >= 0 )
	{
		if(object->GetType(index) == AmfDataType::String && strcmp("avc1", object->GetString(index)) == 0)			
			video_codec_type = CodecType::H264;
		else if(object->GetType(index) == AmfDataType::String && strcmp("H264Avc", object->GetString(index)) == 0)	
			video_codec_type = CodecType::H264;
		else if(object->GetType(index) == AmfDataType::Number && object->GetNumber(index) == 7.0)					
			video_codec_type = CodecType::H264;
	}
	 
	if((index = object->FindName("framerate")) >= 0 && object->GetType(index) == AmfDataType::Number)		
		frame_rate 	=  object->GetNumber(index);  	 
	else if((index = object->FindName("videoframerate")) >= 0 && object->GetType(index) == AmfDataType::Number)	
		frame_rate 	=  object->GetNumber(index);  
	 
	if((index = object->FindName("width")) >= 0 && object->GetType(index) == AmfDataType::Number)	
		video_width =  object->GetNumber(index); 	//Width

	if((index = object->FindName("height")) >= 0 && object->GetType(index) == AmfDataType::Number)	
		video_height =  object->GetNumber(index); 	//Height
	 
	if((index = object->FindName("videodatarate")) >= 0 && object->GetType(index) == AmfDataType::Number)	
		video_bitrate	= object->GetNumber(index); 	// Video Data Rate

	if((index = object->FindName("bitrate")) >= 0 && object->GetType(index) == AmfDataType::Number)			
		video_bitrate	= object->GetNumber(index); 	// Video Data Rate

	if(((index = object->FindName("maxBitrate")) >= 0) && object->GetType(index) == AmfDataType::String)
	{
		bitrate_string = object->GetString(index); 
		video_bitrate = atoi(bitrate_string.c_str()); 
	}
	 
	if((index = object->FindName("audiocodecid")) >= 0 )																
	{
		if		(object->GetType(index) == AmfDataType::String && strcmp("mp4a", object->GetString(index)) == 0)	audio_codec_type = CodecType::AAC;	//AAC	
		else if	(object->GetType(index) == AmfDataType::String && strcmp("mp3 ", object->GetString(index)) == 0)	audio_codec_type = CodecType::MP3;	//MP3	
		else if	(object->GetType(index) == AmfDataType::String && strcmp(".mp3", object->GetString(index)) == 0)	audio_codec_type = CodecType::MP3;	//MP3	
		else if	(object->GetType(index) == AmfDataType::String && strcmp("speex", object->GetString(index)) == 0)	audio_codec_type = CodecType::SPEEX;//Speex	
		else if	(object->GetType(index) == AmfDataType::Number && object->GetNumber(index) == 10.0) 				audio_codec_type = CodecType::AAC;//Speex
		else if	(object->GetType(index) == AmfDataType::Number && object->GetNumber(index) == 11.0) 				audio_codec_type = CodecType::SPEEX;//Speex
		else if	(object->GetType(index) == AmfDataType::Number && object->GetNumber(index) == 2.0) 					audio_codec_type = CodecType::MP3; 	//MP3
	}

	if((index = object->FindName("audiodatarate")) >= 0 && object->GetType(index) 	== AmfDataType::Number)	audio_bitrate =  object->GetNumber(index);	// Audio Data Rate
	if((index = object->FindName("audiobitrate")) >= 0  && object->GetType(index) 	== AmfDataType::Number)	audio_bitrate =  object->GetNumber(index); 	// Audio Data Rate
	 
	if((index = object->FindName("audiochannels")) >= 0 )
	{
		if		(object->GetType(index) == AmfDataType::Number)														audio_channels =  object->GetNumber(index); 	
		else if	(object->GetType(index) == AmfDataType::String && strcmp("stereo", object->GetString(index)) == 0)	audio_channels	 	= 2;
		else if	(object->GetType(index) == AmfDataType::String && strcmp("mono", object->GetString(index)) == 0)		audio_channels 		= 1;
	}

	// Audio samplerate
	if ((index = object->FindName("audiosamplerate")) >= 0)   audio_samplerate = object->GetNumber(index); 

	// Audio samplesize
	if ((index = object->FindName("audiosamplesize")) >= 0)	  audio_samplesize = object->GetNumber(index); 

	_media_info.video_streaming		= (video_codec_type == CodecType::H264);
	_media_info.audio_streaming		= (audio_codec_type == CodecType::AAC);
	_media_info.video_codec_type	= video_codec_type;
	_media_info.video_width			= (int32_t)video_width;
	_media_info.video_height		= (int32_t)video_height;
	_media_info.video_framerate		= (float)frame_rate;
	_media_info.video_bitrate		= (int32_t)video_bitrate;
	_media_info.audio_codec_type	= audio_codec_type;
	_media_info.audio_bitrate		= (int32_t)audio_bitrate;
	_media_info.audio_channels		= (int32_t)audio_channels;
	_media_info.audio_bits			= (int32_t)audio_samplesize;
	_media_info.audio_samplerate	= (int32_t)audio_samplerate;
	_media_info.encoder_type		= encoder_type;
	
	LOG_WRITE(("\n======= MEDIA INFO ======= \n"\
		"encoder : %d(%s) \n"\
		"video : %d/%d*%d/%.2ffps/%dkbps \n"\
		"audio : %d/%dch/%dhz/%dkbps \n",
		encoder_type,
		device_type_string.c_str(),
		_media_info.video_codec_type,
		_media_info.video_width,
		_media_info.video_height,
		_media_info.video_framerate,
		_media_info.video_bitrate,
		_media_info.audio_codec_type,
		_media_info.audio_channels,
		_media_info.audio_samplerate,
		_media_info.audio_bitrate));
		
	return true;
}
/*Meta Data Info
==============================  OBS  ==============================
duration : 0.0
fileSize : 0.0
width : 1280.0
height : 720.0
videocodecid : avc1
videodatarate : 2500.0
framerate : 30.0
audiocodecid : mp4a
audiodatarate : 160.0
audiosamplerate : 44100.0
audiosamplesize : 16.0
audiochannels : 2.0
stereo : 1
2.1 : 0
3.1 : 0
4.0 : 0
4.1 : 0
5.1 : 0
7.1 : 0
encoder : obs-output module (libobs version 22.0.2)

==============================  XSplit  ==============================
author : SIZE-ZERO STRING
copyright : SIZE-ZERO STRING
description : SIZE-ZERO STRING
keywords : SIZE-ZERO STRING
rating : SIZE-ZERO STRING
title : SIZE-ZERO STRING
presetname : Custom
creationdate : Tue Sep 11 10:57:23 2018
videodevice : VHVideoCustom
framerate : 30.0
width : 1280.0
height : 720.0
videocodecid : avc1
avclevel : 31.0
avcprofile : 100.0
videodatarate : 2929.7
audiodevice : VHAudioCustom
audiosamplerate : 48000.0
audiochannels : 1.0
audioinputvolume : 100.0
audiocodecid : mp4a
audiodatarate : 93.8
aid : 2gZQW8H4TOTc4QZ7w2yXfw==
bufferSize : 3000k
maxBitrate : 3000k
pluginName : CustomRTMP
pluginVersion : 3.4.1805.1801
=> Object  End  <=

==============================  ffmpeg  ==============================
duration : 0.0
width : 1280.0
height : 720.0
videodatarate : 0.0
framerate : 24.0
videocodecid : 7.0
audiodatarate : 125.0
audiosamplerate : 44100.0
audiosamplesize : 16.0
stereo : 1
audiocodecid : 10.0
major_brand : mp42
minor_version : 0
compatible_brands : isommp42
encoder : Lavf58.12.100
filesize : 0.0

*/