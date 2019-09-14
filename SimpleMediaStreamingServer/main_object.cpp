//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "main_object.h"
#include <iomanip>

enum class TimerId
{
	GarbageCheck,
	InfoPrint,
 
};

#define GARBAGE_CHECK_INTERVAL (20)
#define INFO_PRINT_INTERVAL (30)

//===============================================================================================
// GetNetworkObjectName
//===============================================================================================
std::string MainObject::GetNetworkObjectName(NetworkObjectKey object_key)
{
	std::string object_key_string;

	switch (object_key)
	{
	case NetworkObjectKey::RtmpEncoder:
		object_key_string = "RtmpEncoder";
		break;

	case NetworkObjectKey::HttpClient:
		object_key_string = "HttpClient";
		break;

	default:
		object_key_string = "unknown";
		break;
	}

	return object_key_string;
}

//====================================================================================================
// Constructor
//====================================================================================================
MainObject::MainObject()
{
	_rtmp_encoder_manager = std::make_shared<RtmpEncoderManager>((int)NetworkObjectKey::RtmpEncoder);
	_http_client_manager = std::make_shared<HttpClientManager>((int)NetworkObjectKey::HttpClient);

	_network_table[(int)NetworkObjectKey::RtmpEncoder] = _rtmp_encoder_manager;
	_network_table[(int)NetworkObjectKey::HttpClient] = _http_client_manager;	
}

//====================================================================================================
// Destructor
//====================================================================================================
MainObject::~MainObject()
{
	Destroy();
}

//====================================================================================================
// Destroy 
//====================================================================================================
void MainObject::Destroy()
{
	_timer.Destroy();

	// network close
	for (int index = 0; index < (int)NetworkObjectKey::Max; index++)
	{
		_network_table[index]->PostRelease();
	}
	LOG_INFO_WRITE(("Network Object Close Completed"));

	// network service close
	if (_network_context_pool != nullptr)
	{
		_network_context_pool->Stop();

		LOG_INFO_WRITE(("Network Service Pool Close Completed"));
	}
}

//====================================================================================================
// Create
//====================================================================================================
bool MainObject::Create(std::unique_ptr<CreateParam> create_param)
{
	_create_param = std::move(create_param);

	// stream manager
	_stream_manager = std::make_shared<StreamManager>(shared_from_this());

	// IoService start
	_network_context_pool = std::make_shared<NetworkContextPool>(_create_param->thread_pool_count);
	_network_context_pool->Run();

	// rtmp encoder
	if (!_rtmp_encoder_manager->Create(std::static_pointer_cast<ITcpNetwork>(this->shared_from_this()),
		_network_context_pool,
		_create_param->rtmp_listen_port,
		GetNetworkObjectName(NetworkObjectKey::RtmpEncoder)))
	{
		LOG_ERROR_WRITE(("[%s] Create Fail", GetNetworkObjectName(NetworkObjectKey::RtmpEncoder).c_str()));
		return false;
	}

	// http client
	if (!_http_client_manager->Create(std::static_pointer_cast<ITcpNetwork>(this->shared_from_this()),
		_network_context_pool,
		_create_param->http_listen_port,
		GetNetworkObjectName(NetworkObjectKey::HttpClient)))
	{
		LOG_ERROR_WRITE(("[%s] Create Fail", GetNetworkObjectName(NetworkObjectKey::HttpClient).c_str()));
		return false;
	}

	// create timer 	
	if (_timer.Create(this) == false || 
		_timer.SetTimer((int)TimerId::GarbageCheck, GARBAGE_CHECK_INTERVAL * 1000) == false,
		_timer.SetTimer((int)TimerId::InfoPrint, INFO_PRINT_INTERVAL * 1000) == false)
	{
		LOG_ERROR_WRITE(("Init Timer Fail"));
		return false;
	}

	return true;
}

//====================================================================================================
// Network Accepted Callback
//====================================================================================================
bool MainObject::OnTcpNetworkAccepted(int object_key, std::shared_ptr<NetTcpSocket> socket, uint32_t ip, int port)
{
	if (object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_ERROR_WRITE(("OnTcpNetworkAccepted - unkown object - obkect_key(%d)", object_key));
		return false;
	}

	int index_key = -1;

	if (object_key == (int)NetworkObjectKey::RtmpEncoder)
	{
		_rtmp_encoder_manager->AcceptedAdd(socket,
			ip,
			port,
			std::static_pointer_cast<IRtmpEncoder>(this->shared_from_this()),
			index_key);
	}
	else if (object_key == (int)NetworkObjectKey::HttpClient)
	{
		_http_client_manager->AcceptedAdd(socket,
			ip,
			port,
			std::static_pointer_cast<IHttpClient>(this->shared_from_this()),
			index_key);
	}


	if (index_key == -1)
	{
		LOG_ERROR_WRITE(("[%s] OnTcpNetworkAccepted - object add fail - index_key(%d) ip(%s)",
			GetNetworkObjectName((NetworkObjectKey)object_key).c_str(),
			index_key,
			GetStringIP(ip).c_str()));
		return false;
	}

	return true;
}

//====================================================================================================
// Network Connected Callback
//====================================================================================================
bool MainObject::OnTcpNetworkConnected(int object_key,
	NetConnectedResult result,
	std::shared_ptr<std::vector<uint8_t>> connected_param,
	std::shared_ptr<NetTcpSocket> socket,
	unsigned ip,
	int port)
{
	if (object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_ERROR_WRITE(("OnTcpNetworkConnected - unkown object - obkect_key(%d)", object_key));
		return false;
	}

	return true;
}

//====================================================================================================
// Network Connection Close Callback
//====================================================================================================
int MainObject::OnNetworkClose(int object_key, int index_key, uint32_t ip, int port)
{
	int stream_key = -1;

	if (object_key >= (int)NetworkObjectKey::Max)
	{
		LOG_ERROR_WRITE(("OnNetworkClose - unkown object - obkect_key(%d)", object_key));
		return 0;
	}

	LOG_INFO_WRITE(("[%s] OnNetworkClose - key(%d) ip(%s) Port(%d)",
		GetNetworkObjectName((NetworkObjectKey)object_key).c_str(), index_key, GetStringIP(ip).c_str(), port));

	// remove session 
	_network_table[object_key]->Remove(index_key);

	return 0;
}

//====================================================================================================
// Delete Session 
//====================================================================================================
bool MainObject::RemoveNetwork(NetworkObjectKey object_key, int index_key)
{
	if (object_key >= NetworkObjectKey::Max)
	{
		LOG_ERROR_WRITE(("RemoveNetwork - obkect_key(%d)", object_key));
		return false;
	}

	// Remove
	_network_table[(int)object_key]->Remove(index_key);

	return true;

}

//====================================================================================================
// Delete Sessions 
//====================================================================================================
bool MainObject::RemoveNetwork(NetworkObjectKey object_key, std::vector<int>& IndexKeys)
{
	if (object_key >= NetworkObjectKey::Max)
	{
		LOG_ERROR_WRITE(("RemoveNetwork - obkect_key(%d)", object_key));
		return false;
	}

	// Remove
	_network_table[(int)object_key]->Remove(IndexKeys);

	return true;
}


//====================================================================================================
// OnRtmpEncoderStart
// - IRtmpEncoder Callback
//====================================================================================================
bool MainObject::OnRtmpEncoderStart(int index_key, uint32_t ip, StreamKey& stream_key)
{
	LOG_INFO_WRITE(("Step 1. Rtmp Encoder Start - stream(%s/%s) ip(%s) index_key(%d)",
					stream_key.first.c_str(), stream_key.second.c_str(), GetStringIP(ip).c_str(), index_key));
	
	if (!_stream_manager->CreateStream(stream_key, index_key, ip))
	{
		LOG_ERROR_WRITE(("OnRtmpEncoderStart - CreateStream fail - stream(%s/%s)", 
						stream_key.first.c_str(), stream_key.second.c_str()));

		return false;
	}
	
	return true;
}

//====================================================================================================
// OnRtmpEncoderReadyComplete
// - IRtmpEncoder Callback
//====================================================================================================
bool MainObject::OnRtmpEncoderReadyComplete(int index_key, uint32_t ip, StreamKey& stream_key, MediaInfo& media_info)
{
	LOG_INFO_WRITE(("Step 2. Remp Encoder Ready Complete - stream(%s/%s) ip(%s) index_key(%d)",
		stream_key.first.c_str(), stream_key.second.c_str(), GetStringIP(ip).c_str(), index_key));

	// compete
	if (!_stream_manager->SetCompleteState(stream_key, media_info))
	{
		LOG_ERROR_WRITE(("OnRtmpEncoderReadyComplete - SetCompleteState fail - stream(%s/%s)", 
						stream_key.first.c_str(), stream_key.second.c_str()));

		return false;
	}

	LOG_INFO_WRITE(("Step 3. Media Info - Stream(%s/%s) *** \n\t\t\t - Video(%dK/%dFps/%d*%d) Audio(%d/%dHz)",
				stream_key.first.c_str(), stream_key.second.c_str(),
				media_info.video_bitrate, (int)media_info.video_framerate, media_info.video_width, media_info.video_height,
				media_info.audio_channels, media_info.audio_samplerate));

	return true;
}

//====================================================================================================
// OnRtmpEncoderStreamData 
// - IRtmpEncoder Callback
//====================================================================================================
bool MainObject::OnRtmpEncoderStreamData(int index_key, uint32_t ip, StreamKey& stream_key, std::shared_ptr<FrameInfo>& frame)
{
	//LOG_INFO_WRITE(("Rtmp Encoder Stream Data - stream(%s/%s) tpye(%c) timestamp(%lld)",
	//				stream_key.first.c_str(), stream_key.second.c_str(), frame->type, frame->timestamp));

	if (!_stream_manager->AppendStreamData(stream_key, frame))
	{
		LOG_ERROR_WRITE(("OnRtmpEncoderStreamData - AppendStreamData fail - Stream(%s/%s)", 
						stream_key.first.c_str(), stream_key.second.c_str()));
		return false;
	}
 
	return true;
}

//====================================================================================================
// OnHttpClientPlaylistRequest 
// - IHttpClient callback
//====================================================================================================
bool MainObject::OnHttpClientPlaylistRequest(int index_key,
											uint32_t ip,
											const StreamKey& stream_key,
											PlaylistType type,
											std::string& playlist)
{
	if (!_stream_manager->GetPlaylist(stream_key, type, playlist))
	{
		LOG_ERROR_WRITE(("OnHttpClientPlaylistRequest - GetHlsPlayList fail - stream(%s/%s)", 
						stream_key.first.c_str(), stream_key.second.c_str()));
		return false;
	}

	return true;
}

//====================================================================================================
// OnHttpClientSegmentRequest 
// - IHttpClient callback
//====================================================================================================
bool MainObject::OnHttpClientSegmentRequest(int index_key,
											uint32_t ip,
											const std::string& file_name,
											const StreamKey& stream_key,
											SegmentType type,
											std::shared_ptr<std::vector<uint8_t>>& data)
{
	if (!_stream_manager->GetSegmentData(stream_key, file_name, type, data))
	{
		LOG_ERROR_WRITE(("OnHttpClientSegmentRequest - GetSegmentData fail - stream(%s/%s) file(%s)", 
						stream_key.first.c_str(), stream_key.second.c_str(), file_name.c_str()));
		return false;
	}

	return true;
}

//====================================================================================================
// Timer Callback
//====================================================================================================
void MainObject::OnThreadTimer(uint32_t timer_id, bool& delete_timer/* = false */)
{
	switch (timer_id)
	{
		case (int)TimerId::GarbageCheck: 					
			GarbageCheckTimerProc();					
			break;

		case (int)TimerId::InfoPrint:
			InfoPrintTimerProc();
			break;
	}
}


//====================================================================================================
// Garbage Check Proc
//====================================================================================================
void MainObject::GarbageCheckTimerProc()
{
	int remove_count = 0;
	std::vector<StreamKey> stream_key_list;
	
	remove_count = _stream_manager->GarbageCheck(stream_key_list);

	if (remove_count > 0)
	{
		LOG_INFO_WRITE(("GarbageCheck - remove(%d)", remove_count));
	}
}

//====================================================================================================
// Information Print Proc
//====================================================================================================
void MainObject::InfoPrintTimerProc()
{
	LOG_INFO_WRITE(("*** Connected - stream(%d) rtmp_encoder(%d) http_client(%d) ***", 
					_stream_manager->GetStreamCount(), 
					_rtmp_encoder_manager->GetCount(), 
					_http_client_manager->GetCount()));
}