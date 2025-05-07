add_library(network STATIC 
	"network_context_pool.cpp"
	"network_context_pool.h"
	"network_manager.cpp"
	"network_manager.h"
	"network_header.h"
	"network_tcp_manager.cpp"
	"network_tcp_manager.h"
	"network_manager.h"
	"network_tcp_object.cpp"
	"network_tcp_object.h"
	"websocket_client.hpp")

include_directories(../)
target_include_directories(network PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(network PUBLIC common  ${Boost_LIBRARIES})