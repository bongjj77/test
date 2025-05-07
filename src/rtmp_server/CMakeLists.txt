project ("rtmp_server")
find_package(Threads REQUIRED)  

add_library(main STATIC 
	"studio/studio_service.cpp"
	"studio/studio_service.h"
	"studio/studio_object.cpp"
	"studio/studio_object.h"
	"studio/studio_stream.cpp"
	"studio/studio_stream.h"
	"player/player_service.cpp"
	"player/player_service.h"
	"player/player_object.cpp"
	"player/player_object.h"
	"player/player_stream.cpp"
	"player/player_stream.h"
	"controller/controller.cpp"
	"controller/controller.h"
	"controller/controller_packet.hpp"
	"main_object.cpp"
	"main_object.h")

include_directories(../)

add_executable(rtmp_server service.cpp)

target_link_libraries(rtmp_server PUBLIC 
  main
  media
  network
  common
  Threads::Threads
  ${Boost_LIBRARIES}
	${HIREDIS_LIBRARIES})
