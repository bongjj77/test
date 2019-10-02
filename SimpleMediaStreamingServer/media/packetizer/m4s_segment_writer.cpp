//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "m4s_segment_writer.h"

#define DEFAULT_SEGMENT_HEADER_SIZE (4096)

//Fragmented MP4
// data m4s
//    - moof(Movie Fragment)
//        - mfnd(Movie Fragment Header)
//        - traf(Track Fragment)
//            - tfhd(Track Fragment Header)
//            - trun(Track Fragment Run)
//            - sdtp(Independent Samples)
//    - mdata(Media Data)

//====================================================================================================
// Constructor
//====================================================================================================
M4sSegmentWriter::M4sSegmentWriter(Mp4MediaType media_type,
									uint32_t sequence_number,
									uint32_t track_id,
									uint64_t start_timestamp) :
									Mp4Writer(media_type)
{
  	_sequence_number    = sequence_number;
	_track_id			= track_id;
	_start_timestamp	= start_timestamp;
}

//====================================================================================================
// Destructor
//====================================================================================================
M4sSegmentWriter::~M4sSegmentWriter( )
{

}

//====================================================================================================
//  CreateData
//====================================================================================================
const std::shared_ptr<std::vector<uint8_t>> M4sSegmentWriter::AppendSamples(const std::vector<std::shared_ptr<SampleData>> &sample_data_list)
{
	uint32_t total_sample_size = 0;

	for(const auto &sample_data : sample_data_list)
	{
		total_sample_size += sample_data->data->size();

		if (_media_type == Mp4MediaType::Video)
		{
			total_sample_size += sizeof(uint32_t);
		}
	}

	auto data_stream = std::make_shared<std::vector<uint8_t>>(total_sample_size + DEFAULT_SEGMENT_HEADER_SIZE);

	MoofBoxWrite(data_stream, sample_data_list);

	MdatBoxWrite(data_stream, sample_data_list, total_sample_size);

	return data_stream;
}

//====================================================================================================
// moof(Movie Fragment) 
//====================================================================================================
int M4sSegmentWriter::MoofBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream, const std::vector<std::shared_ptr<SampleData>> &sample_data_list)
{
	auto data = std::make_shared<std::vector<uint8_t>>(DEFAULT_SEGMENT_HEADER_SIZE);

	MfhdBoxWrite(data);

	TrafBoxWrite(data, sample_data_list);

	BoxDataWrite("moof", data, data_stream);

	uint32_t data_offset = data_stream->size() + 8;

	uint32_t position = (_media_type == Mp4MediaType::Video) ? data_stream->size() - sample_data_list.size() * 16 - 4:
					data_stream->size() - sample_data_list.size() * 8 - 4;

	if(position < 0)
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
int M4sSegmentWriter::MfhdBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream)
{
	auto data = std::make_shared<std::vector<uint8_t>>();

	WriteUint32(_sequence_number, data);	// Sequence Number

	return BoxDataWrite("mfhd", 0, 0, data, data_stream);
}

//====================================================================================================
// traf(Track Fragment) 
//====================================================================================================
int M4sSegmentWriter::TrafBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream, const std::vector<std::shared_ptr<SampleData>> &sample_data_list)
{
	auto data = std::make_shared<std::vector<uint8_t>>();

	TfhdBoxWrite(data);
	TfdtBoxWrite(data);
	TrunBoxWrite(data, sample_data_list);

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

int M4sSegmentWriter::TfhdBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream)
{
	auto data = std::make_shared<std::vector<uint8_t>>();
	uint32_t flag = TFHD_FLAG_DEFAULT_BASE_IS_MOOF;

	WriteUint32(_track_id, data);	// track id

	return BoxDataWrite("tfhd", 0, flag, data, data_stream);
}

//====================================================================================================
// tfdt(Track Fragment Base Media Decode Time) 
//====================================================================================================
int M4sSegmentWriter::TfdtBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream)
{
	auto data = std::make_shared<std::vector<uint8_t>>();
	
	WriteUint64(_start_timestamp, data);    // Base media decode time

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

int M4sSegmentWriter::TrunBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream,
									const std::vector<std::shared_ptr<SampleData>> &sample_data_list)
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

	WriteUint32(sample_data_list.size(), data);	// Sample Item Count;
	WriteUint32(0x11111111, data);	            // Data offset - temp 0 setting

	for (auto &sample_data : sample_data_list)
	{
		WriteUint32(sample_data->duration, data);					// duration

		if (_media_type == Mp4MediaType::Video)
		{
			WriteUint32(sample_data->data->size() + 4, data);			// size + sample
			WriteUint32(sample_data->flag, data);;						// flag
			WriteUint32(sample_data->composition_time_offset, data);	// compoistion timeoffset 
		}
		else if (_media_type == Mp4MediaType::Audio)
		{
			WriteUint32(sample_data->data->size(), data);				// sample
		}
	}

	return BoxDataWrite("trun", 0, flag, data, data_stream);
}

//====================================================================================================
// mdat(Media Data) 
//====================================================================================================
int M4sSegmentWriter::MdatBoxWrite(std::shared_ptr<std::vector<uint8_t>> &data_stream,
									const std::vector<std::shared_ptr<SampleData>> &sample_data_list,
									uint32_t total_sample_size)
{
	WriteUint32(MP4_BOX_HEADER_SIZE + total_sample_size, data_stream); 	// box size write
	WriteText("mdat", data_stream);			// type write

	for (auto &sample_data : sample_data_list)
	{
		// only video)
		if (_media_type == Mp4MediaType::Video)
		{
			WriteUint32(sample_data->data->size(), data_stream);
		}

		WriteData(sample_data->data, data_stream);
	}

	return data_stream->size();
}