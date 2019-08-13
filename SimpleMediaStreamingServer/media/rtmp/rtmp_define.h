
//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once

#include "../media_define.h"


#pragma pack(1)

#define RTMP_SESSION_KEY_MAX_SIZE               (1024)
#define RTMP_TIMESCALE                          (1000)

// HANDSHAKE
#define RTMP_HANDSHAKE_VERSION                  (0x03)
#define RTMP_HANDSHAKE_PACKET_SIZE              (1536)

// CHUNK
#define RTMP_CHUNK_BASIC_HEADER_SIZE_MAX        (3)
#define RTMP_CHUNK_MSG_HEADER_SIZE_MAX          (11 + 4)    // header + extended timestamp
#define RTMP_PACKET_HEADER_SIZE_MAX             (RTMP_CHUNK_BASIC_HEADER_SIZE_MAX + RTMP_CHUNK_MSG_HEADER_SIZE_MAX)

#define RTMP_CHUNK_BASIC_FORMAT_TYPE_MASK       (0xc0)
#define RTMP_CHUNK_BASIC_CHUNK_STREAM_ID_MASK   (0x3f)

#define RTMP_CHUNK_BASIC_FORMAT_TYPE0           (0x00)
#define RTMP_CHUNK_BASIC_FORMAT_TYPE1           (0x40)
#define RTMP_CHUNK_BASIC_FORMAT_TYPE2           (0x80)
#define RTMP_CHUNK_BASIC_FORMAT_TYPE3           (0xc0)

#define RTMP_CHUNK_BASIC_FORMAT_TYPE0_SIZE      (11)
#define RTMP_CHUNK_BASIC_FORMAT_TYPE1_SIZE      (7)
#define RTMP_CHUNK_BASIC_FORMAT_TYPE2_SIZE      (3)
#define RTMP_CHUNK_BASIC_FORMAT_TYPE3_SIZE      (0)

#define RTMP_EXTEND_TIMESTAMP                   (0x00ffffff)
#define RTMP_EXTEND_TIMESTAMP_SIZE              (4)

// CHUNK STREAM ID
#define RTMP_CHUNK_STREAM_ID_URGENT             (2)
#define RTMP_CHUNK_STREAM_ID_CONTROL            (3)
#define RTMP_CHUNK_STREAM_ID_MEDIA              (8)

// MESSAGE ID
#define RTMP_MSGID_SET_CHUNK_SIZE               (1)
#define RTMP_MSGID_ABORT_MESSAGE                (2)
#define RTMP_MSGID_ACKNOWLEDGEMENT              (3)
#define RTMP_MSGID_USER_CONTROL_MESSAGE         (4)
#define RTMP_MSGID_WINDOWACKNOWLEDGEMENT_SIZE   (5)
#define RTMP_MSGID_SET_PEERBANDWIDTH            (6)
#define RTMP_MSGID_AUDIO_MESSAGE                (8)
#define RTMP_MSGID_VIDEO_MESSAGE                (9)
#define RTMP_MSGID_AMF3_DATA_MESSAGE            (15)
#define RTMP_MSGID_AMF3_COMMAND_MESSAGE         (17)
#define RTMP_MSGID_AMF0_DATA_MESSAGE            (18)
#define RTMP_MSGID_AMF0_COMMAND_MESSAGE         (20)
#define RTMP_MSGID_AGGREGATE_MESSAGE            (22)

// USER CONTROL MESSAGE ID
#define RTMP_UCMID_STREAMBEGIN                  (0)
#define RTMP_UCMID_STREAMEOF                    (1)
#define RTMP_UCMID_STREAMDRY                    (2)
#define RTMP_UCMID_SETBUFFERLENGTH              (3)
#define RTMP_UCMID_STREAMISRECORDED             (4)
#define RTMP_UCMID_PINGREQUEST                  (6)
#define RTMP_UCMID_PINGRESPONSE                 (7)
#define RTMP_UCMID_BUFFEREMPTY                  (31)
#define RTMP_UCMID_BUFFERREADY                  (32)

// COMMAND TRANSACTION ID
#define RTMP_CMD_TRID_NOREPONSE                 (0.0)
#define RTMP_CMD_TRID_CONNECT                   (1.0)
#define RTMP_CMD_TRID_CREATESTREAM              (2.0)
#define RTMP_CMD_TRID_RELEASESTREAM             (2.0)
#define RTMP_CMD_TRID_DELETESTREAM              RTMP_CMD_TRID_NOREPONSE
#define RTMP_CMD_TRID_CLOSESTREAM               RTMP_CMD_TRID_NOREPONSE
#define RTMP_CMD_TRID_PLAY                      RTMP_CMD_TRID_NOREPONSE
#define RTMP_CMD_TRID_PLAY2                     RTMP_CMD_TRID_NOREPONSE
#define RTMP_CMD_TRID_SEEK                      RTMP_CMD_TRID_NOREPONSE
#define RTMP_CMD_TRID_PAUSE                     RTMP_CMD_TRID_NOREPONSE
#define RTMP_CMD_TRID_RECEIVEAUDIO              RTMP_CMD_TRID_NOREPONSE
#define RTMP_CMD_TRID_RECEIVEVIDEO              RTMP_CMD_TRID_NOREPONSE
#define RTMP_CMD_TRID_PUBLISH                   RTMP_CMD_TRID_NOREPONSE
#define RTMP_CMD_TRID_FCPUBLISH					(3.0)
#define RTMP_CMD_TRID_FCSUBSCRIBE               (4.0)
#define RTMP_CMD_TRID_FCUNSUBSCRIBE             (5.0)
#define RTMP_CMD_TRID_FCUNPUBLISH               (5.0)

// COMMAND NAME
#define RTMP_CMD_NAME_CONNECT                   ("connect")
#define RTMP_CMD_NAME_CREATESTREAM              ("createStream")
#define RTMP_CMD_NAME_RELEASESTREAM             ("releaseStream")
#define RTMP_CMD_NAME_DELETESTREAM              ("deleteStream")
#define RTMP_CMD_NAME_CLOSESTREAM               ("closeStream")
#define RTMP_CMD_NAME_PLAY                      ("play")
#define RTMP_CMD_NAME_ONSTATUS                  ("onStatus")
#define RTMP_CMD_NAME_PUBLISH                   ("publish")
#define RTMP_CMD_NAME_FCPUBLISH					("FCPublish")
#define RTMP_CMD_NAME_FCUNPUBLISH               ("FCUnpublish")
#define RTMP_CMD_NAME_ONFCPUBLISH               ("onFCPublish")
#define RTMP_CMD_NAME_ONFCUNPUBLISH				("onFCUnpublish")
#define RTMP_CMD_NAME_CLOSE						("close")
#define RTMP_CMD_NAME_SETCHALLENGE              ("setChallenge")
#define RTMP_CMD_DATA_SETDATAFRAME              ("@setDataFrame")
#define RTMP_CMD_NAME_ONCLIENTLOGIN             ("onClientLogin")
#define RTMP_CMD_DATA_ONMETADATA                ("onMetaData")
#define RTMP_ACK_NAME_RESULT                    ("_result")
#define RTMP_ACK_NAME_ERROR						("_error")
#define RTMP_ACK_NAME_BWDONE					("onBWDone")
#define RTMP_PING                               ("ping")

//RtmpStream/RtmpPublish 
#define RTMP_H264_I_FRAME_TYPE                  (0x17)
#define RTMP_H264_P_FRAME_TYPE                  (0x27)
#define RTMP_DEFAULT_ACKNOWNLEDGEMENT_SIZE      (2500000)
#define RTMP_DEFAULT_PEER_BANDWIDTH             (2500000)
#define RTMP_DEFAULT_CHUNK_SIZE                 (128)

#define RTMP_SEQUENCE_INFO_TYPE					(0x00)
#define RTMP_FRAME_DATA_TYPE                    (0x01)
#define RTMP_VIDEO_DATA_MIN_SIZE                (5) // contdrol(1) + sequece(1) + offsettime(3)

#define RTMP_AUDIO_CONTROL_HEADER_INDEX         (0)
#define RTMP_AAC_AUDIO_SEQUENCE_HEADER_INDEX	(1)
#define RTMP_AAC_AUDIO_DATA_MIN_SIZE            (2) // contdrol(1) + sequece(1)
#define RTMP_AAC_AUDIO_FRAME_INDEX              (2)
#define RTMP_SPEEX_AUDIO_FRAME_INDEX            (1)
#define RTMP_MP3_AUDIO_FRAME_INDEX              (1)
#define RTMP_SPS_PPS_MIN_DATA_SIZE              (14)

#define RTMP_VIDEO_CONTROL_HEADER_INDEX         (0)
#define RTMP_VIDEO_SEQUENCE_HEADER_INDEX        (1)
#define RTMP_VIDEO_COMPOSITION_OFFSET_INDEX     (2)
#define RTMP_VIDEO_FRAME_SIZE_INDEX             (5)
#define RTMP_VIDEO_FRAME_INDEX                  (9)
#define RTMP_VIDEO_FRAME_SIZE_INFO_SIZE         (4)

#define RTMP_UNKNOWN_DEVICE_TYPE_STRING         ("unknown_device_type")
#define RTMP_DEFAULT_CLIENT_VERSION				("rtmp client 1.0")//"afcCli 11,0,100,1 (compatible; FMSc/1.0)"
#define RTMP_DEFULT_PORT                        (1935)

//====================================================================================================
// chunk_header 
//====================================================================================================
struct RtmpChunkHeader 
{
public:
	RtmpChunkHeader() 
	{
		basic_header.format_type = 0;
		basic_header.chunk_stream_id = 0;

		type_0.timestamp = 0;
		type_0.body_size = 0;
		type_0.type_id = 0;
		type_0.stream_id = 0;
	}

	void Init() 
	{
		basic_header.format_type = 0;
		basic_header.chunk_stream_id = 0;

		type_0.timestamp = 0;
		type_0.body_size = 0;
		type_0.type_id = 0;
		type_0.stream_id = 0;
	}

public:
	struct 
	{
		uint8_t		format_type;
		uint32_t	chunk_stream_id; // 0,1,2 are reserved
	} basic_header;

	union 
	{
		//
		struct 
		{
			uint32_t	timestamp;
			uint32_t	body_size;          
			uint8_t		type_id;            
			uint32_t	stream_id;          
		} type_0;
		//
		struct 
		{
			uint32_t	timestamp_delta;
			uint32_t	body_size;
			uint8_t		type_id;
		} type_1;
		//
		struct 
		{
			uint32_t	timestamp_delta;
		} type_2;
	};
};

//===============================================================================================
// Rtmp Handshake State
//===============================================================================================
enum class RtmpHandshakeState 
{
	Ready = 0,
	C0,
	C1,
	S0,
	S1,
	S2,
	C2,
	Complete,
};

#pragma pack()