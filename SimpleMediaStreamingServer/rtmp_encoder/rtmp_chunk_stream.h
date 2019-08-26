//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once
#include <basetsd.h>
#include <map>
#include <vector>
#include <time.h>

#include "media/rtmp/rtmp_import_chunk.h"
#include "media/rtmp/rtmp_export_chunk.h"
#include "media/rtmp/amf_document.h"
#include "media/rtmp/rtmp_handshake.h"

//====================================================================================================
// Interface 
//====================================================================================================
class IRtmpChunkStream
{
public:

	virtual bool OnChunkStreamSend(int data_size, char * data) = 0;	
	virtual bool OnChunkStreamStart(StreamKey & stream_key) = 0;
	virtual bool OnChunkStreamReadyComplete(StreamKey & stream_key, MediaInfo & media_info) = 0;
	virtual bool OnChunkStreamData(StreamKey & stream_key, std::shared_ptr<FrameInfo> &frame) = 0;
	
};

//====================================================================================================
// RtmpChunkStream
//====================================================================================================
class RtmpChunkStream
{
public:
	RtmpChunkStream();
	virtual ~RtmpChunkStream();


public : 
	bool	Create(IRtmpChunkStream * stream_interface, int chunk_size = 4096);
	void	Destroy(); 
	
	StreamKey & GetStreamKey(){ return _stream_key; }
	std::string	GetApplicationName(){ return _app_name; }
	std::string	GetStreamName(){ return _app_stream_name; }
	int			OnDataReceived(int data_size, char * data);
	
private:
	int32_t	ReceiveHandshakePacket(const std::shared_ptr<const std::vector<uint8_t>> &data);
	int32_t	ReceiveChunkPacket(const std::shared_ptr<const std::vector<uint8_t>> &data);
		
	bool	SendData(uint32_t data_size, uint8_t * data);

	bool	SendHandshake(const std::shared_ptr<const std::vector<uint8_t>> &data);

	bool	ReceiveChunkMessage();
	bool	ReceiveSetChunkSize(std::shared_ptr<ImportMessage> &message);
	void	ReceiveWindowAcknowledgementSize(std::shared_ptr<ImportMessage> &message);
	void	ReceiveAmfCommandMessage(std::shared_ptr<ImportMessage> &message);
	void	ReceiveAmfDataMessage(std::shared_ptr<ImportMessage> &message);
	bool	ReceiveAudioMessage(std::shared_ptr<ImportMessage> &message);
	bool	ReceiveVideoMessage(std::shared_ptr<ImportMessage> &message);
	
	void 	OnAmfConnect(std::shared_ptr<RtmpMuxMessageHeader> &message_header, AmfDocument &document, double transaction_id);
	void 	OnAmfCreateStream(std::shared_ptr<RtmpMuxMessageHeader> &message_header, AmfDocument &document, double transaction_id);
	void 	OnAmfFCPublish(std::shared_ptr<RtmpMuxMessageHeader> &message_header, AmfDocument &document, double transaction_id);
	void 	OnAmfPublish(std::shared_ptr<RtmpMuxMessageHeader> &message_header, AmfDocument &document, double transaction_id);
	void 	OnAmfDeleteStream(std::shared_ptr<RtmpMuxMessageHeader> &message_header, AmfDocument &document, double transaction_id);
	bool 	OnAmfMetaData(std::shared_ptr<RtmpMuxMessageHeader> &message_header, AmfDocument &document, int32_t object_index);
	
	bool	SendMessagePacket(std::shared_ptr<RtmpMuxMessageHeader> &message_header, std::shared_ptr<std::vector<uint8_t>> &data);
	bool	SendUserControlMessage(uint16_t message, std::shared_ptr<std::vector<uint8_t>> &data);

	bool	SendWindowAcknowledgementSize();
	bool	SendSetPeerBandwidth();
	bool	SendStreamBegin();
	bool	SendAcknowledgementSize();

	bool	SendAmfCommand(std::shared_ptr<RtmpMuxMessageHeader> &message_header, AmfDocument &document);
	bool	SendAmfConnectResult(uint32_t chunk_stream_id, double transaction_id, double object_encoding);
	bool	SendAmfOnFCPublish(uint32_t chunk_stream_id, uint32_t stream_id, double client_id);
	bool	SendAmfCreateStreamResult(uint32_t chunk_stream_id, double transaction_id);
	bool	SendAmfOnStatus(uint32_t chunk_stream_id, uint32_t stream_id, char * level, char * code, char * description, double client_id);

	bool	ProcessVideoSequenceData(std::unique_ptr<std::vector<uint8_t>> data);
	bool 	ProcessAudioSequenceData(std::unique_ptr<std::vector<uint8_t>> data);

	bool	StreamInfoLoadCheck();


private : 

	StreamKey 	_stream_key;
	std::string	_app_name;
	std::string _app_stream_name;
	
	IRtmpChunkStream * _stream_interface;
	RtmpHandshakeState _handshake_state;
	std::unique_ptr<RtmpImportChunk> _import_chunk;
	std::unique_ptr<RtmpExportChunk> _export_chunk;
	MediaInfo	_media_info;
	uint32_t	_stream_id;
	uint32_t	_peer_bandwidth;
	uint32_t	_acknowledgement_size;
	uint32_t	_acknowledgement_traffic; 
	double		_client_id; 
	int			_chunk_stream_id; 
	bool		_video_sequence_load;
	bool		_audio_sequence_load;
	bool		_frame_config_info_load; 
};
