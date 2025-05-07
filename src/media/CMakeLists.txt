add_library(media STATIC 
	"rtmp/amf_document.cpp"
	"rtmp/amf_document.h"
	"rtmp/amf_util.cpp"
	"rtmp/amf_util.h"
	"rtmp/rtmp_define.h"
	"rtmp/rtmp_media_parser.cpp"
	"rtmp/rtmp_media_parser.h"
	"rtmp/rtmp_export_chunk.cpp"
	"rtmp/rtmp_export_chunk.h"
	"rtmp/rtmp_handshake.cpp"
	"rtmp/rtmp_handshake.h"
	"rtmp/rtmp_import_chunk.cpp"
	"rtmp/rtmp_import_chunk.h"
	"rtmp/rtmp_mux_util.cpp"
	"rtmp/rtmp_mux_util.h"
)

include_directories(../)

target_link_libraries(media PUBLIC common)