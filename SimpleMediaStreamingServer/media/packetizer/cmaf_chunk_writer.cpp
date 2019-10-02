//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "cmaf_chunk_writer.h"

#define DEFAULT_CHUNK_HEADER_SIZE (256)

//Fragmented MP4
// data m4s
//    - moof(Movie Fragment)
//        - mfnd(Movie Fragment Header)
//        - traf(Track Fragment)
//            - tfhd(Track Fragment Header)
//            - trun(Track Fragment Run)
//            - sdtp(Independent Samples)
//    - mdata(Media Data)
//    ...
//

//====================================================================================================
// Constructor
//====================================================================================================
CmafChunkWriter::CmafChunkWriter(Mp4MediaType media_type,
											 uint32_t sequence_number,
											 uint32_t track_id,
											 bool http_chunked_transfer_support)
												: Mp4Writer(media_type)
{
  	_sequence_number = sequence_number;
	_track_id = track_id;
	_http_chunked_transfer_support = http_chunked_transfer_support;
}

//====================================================================================================
// Destructor
//====================================================================================================
CmafChunkWriter::~CmafChunkWriter( )
{
}
//====================================================================================================
//  GetHttpChunkedSegmentBody
// - make http chunked transfer body
//====================================================================================================
std::shared_ptr<std::vector<uint8_t>> CmafChunkWriter::GetChunkedSegment()
{
	if(_chunked_data->size() <= 0)
		return nullptr;

	auto chunked_segment = _chunked_data;

	if(_http_chunked_transfer_support)
	{
		std::string suffix = "0\r\n\r\n";
		chunked_segment->insert(chunked_segment->end(), suffix.begin(), suffix.end());
	}

	if(_max_chunked_data_size < chunked_segment->size())
		_max_chunked_data_size = chunked_segment->size();

	return chunked_segment;
}

//====================================================================================================
//  AppendSample
//====================================================================================================
const std::shared_ptr<std::vector<uint8_t>> CmafChunkWriter::AppendSample(const std::shared_ptr<SampleData> &sample_data)
{
    if(!_write_started)
    {
        _write_started = true;
        _start_timestamp = sample_data->timestamp;

        if(_chunked_data == nullptr)
			_chunked_data = std::make_shared<std::vector<uint8_t>>(_max_chunked_data_size);
    }

	auto chunk_stream = std::make_shared<std::vector<uint8_t>>(sample_data->data->size() + DEFAULT_CHUNK_HEADER_SIZE);

    // moof box write
    MoofBoxWrite(chunk_stream, sample_data);

    // mdata box write
    MdatBoxWrite(chunk_stream, sample_data->data);

    if(_http_chunked_transfer_support)
	{
		std::string prefix = string_format("%x\r\n", chunk_stream->size());
		std::string suffix = "\r\n";

		chunk_stream->insert(chunk_stream->begin(), prefix.begin(), prefix.end());
		chunk_stream->insert(chunk_stream->end(), suffix.begin(), suffix.end());
	}

	_chunked_data->insert(_chunked_data->end(), chunk_stream->begin(), chunk_stream->end());

	_sample_count++;

	return chunk_stream;
}

//====================================================================================================
// moof(Movie Fragment)
//====================================================================================================
int CmafChunkWriter::MoofBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream,
									const std::shared_ptr<SampleData> &sample_data)
{
	auto data = std::make_shared<std::vector<uint8_t>>(DEFAULT_CHUNK_HEADER_SIZE);

	MfhdBoxWrite(data);
	TrafBoxWrite(data, sample_data);

	BoxDataWrite("moof", data, data_stream);

	uint32_t data_offset = data_stream->size() + 8;

	uint32_t position = (_media_type == Mp4MediaType::Video) ? data_stream->size() - 16 - 4 : data_stream->size() - 8 - 4;

	if (position < 0)
	{
		return -1;
	}

	// trun data offset value change
	(*data_stream)[position] = ((uint8_t)(data_offset >> 24 & 0xFF));
	(*data_stream)[++position] = ((uint8_t)(data_offset >> 16 & 0xFF));
	(*data_stream)[++position] = ((uint8_t)(data_offset >> 8 & 0xFF));
	(*data_stream)[++position] = ((uint8_t)(data_offset & 0xFF));

	return data_stream->size();
}

//====================================================================================================
// mfhd(Movie Fragment Header)
//====================================================================================================
int CmafChunkWriter::MfhdBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream)
{
	auto data = std::make_shared<std::vector<uint8_t>>();

	WriteUint32(_sequence_number++, data);

	return BoxDataWrite("mfhd", 0, 0, data, data_stream);
}

//====================================================================================================
// traf(Track Fragment)
//====================================================================================================
int CmafChunkWriter::TrafBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream,
									const std::shared_ptr<SampleData> &sample_data)
{
	auto data = std::make_shared<std::vector<uint8_t>>();

	TfhdBoxWrite(data);
	TfdtBoxWrite(data, sample_data->timestamp);
	TrunBoxWrite(data, sample_data);

	return BoxDataWrite("traf", data, data_stream);
}

//====================================================================================================
// tfhd(Track Fragment Header)
//====================================================================================================
#define TFHD_FLAG_BASE_DATA_OFFSET_PRESENT          (0x00001)
#define TFHD_FLAG_SAMPLE_DESCRIPTION_INDEX_PRESENT  (0x00002)
#define TFHD_FLAG_DEFAULT_SAMPLE_DURATION_PRESENT   (0x00008)
#define TFHD_FLAG_DEFAULT_SAMPLE_SIZE_PRESENT       (0x00010)
#define TFHD_FLAG_DEFAULT_SAMPLE_FLAGS_PRESENT      (0x00020)
#define TFHD_FLAG_DURATION_IS_EMPTY                 (0x10000)
#define TFHD_FLAG_DEFAULT_BASE_IS_MOOF              (0x20000)

int CmafChunkWriter::TfhdBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream)
{
	auto data = std::make_shared<std::vector<uint8_t>>();
	uint32_t flag = TFHD_FLAG_DEFAULT_BASE_IS_MOOF;

	WriteUint32(_track_id, data);	// track id

	return BoxDataWrite("tfhd", 0, flag, data, data_stream);
}

//====================================================================================================
// tfdt(Track Fragment Base Media Decode Time)
//====================================================================================================
int CmafChunkWriter::TfdtBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream, uint64_t timestamp)
{
	auto data = std::make_shared<std::vector<uint8_t>>();

	WriteUint64(timestamp, data);    // Base media decode time

	return BoxDataWrite("tfdt", 1, 0, data, data_stream);
}

//====================================================================================================
// Trun(Track Fragment Run)
//====================================================================================================
#define TRUN_FLAG_DATA_OFFSET_PRESENT                    (0x0001)
#define TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT             (0x0004)
#define TRUN_FLAG_SAMPLE_DURATION_PRESENT                (0x0100)
#define TRUN_FLAG_SAMPLE_SIZE_PRESENT                    (0x0200)
#define TRUN_FLAG_SAMPLE_FLAGS_PRESENT                   (0x0400)
#define TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSET_PRESENT (0x0800)

int CmafChunkWriter::TrunBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream,
									const std::shared_ptr<SampleData> &sample_data)
{
	auto data = std::make_shared<std::vector<uint8_t>>();
	uint32_t flag = 0;

	if (Mp4MediaType::Video == _media_type)
	{
		flag = TRUN_FLAG_DATA_OFFSET_PRESENT | TRUN_FLAG_SAMPLE_DURATION_PRESENT | TRUN_FLAG_SAMPLE_SIZE_PRESENT |
		        TRUN_FLAG_SAMPLE_FLAGS_PRESENT | TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSET_PRESENT;
	}
	else if(Mp4MediaType::Audio == _media_type)
	{
		flag = TRUN_FLAG_DATA_OFFSET_PRESENT | TRUN_FLAG_SAMPLE_DURATION_PRESENT | TRUN_FLAG_SAMPLE_SIZE_PRESENT;
	}

	WriteUint32(1, data);	                // Sample Item Count;
	WriteUint32(0x11111111, data);	         // Data offset - temp 0 setting

    WriteUint32(sample_data->duration, data); // duration

    if (_media_type == Mp4MediaType::Video)
    {
        WriteUint32(sample_data->data->size() + 4, data);	// size + sample
        WriteUint32(sample_data->flag, data);;					// flag
        WriteUint32(sample_data->composition_time_offset, data);	// cts
    }
    else if (_media_type == Mp4MediaType::Audio)
    {
        WriteUint32(sample_data->data->size(), data);				// sample
    }

	return BoxDataWrite("trun", 0, flag, data, data_stream);
}

//====================================================================================================
// mdat(Media Data)
//====================================================================================================
int CmafChunkWriter::MdatBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream,
										const std::shared_ptr<std::vector<uint8_t>> &frame)
{
	return BoxDataWrite("mdat", frame, data_stream, _media_type == Mp4MediaType::Video ? true : false);

}