.SUFFIXES : .cpp .o

VPATH =		../:\
			../../common:\
			../../common/network/engine:\
			../test_tcp_client
				
CXX = g++

INCLUDE =	-I../\
			-I../../

BASE_LIBS = -lpthread

DEFINES = -D_THREAD_SAFE

BOOST_INCLUDE = -I/usr/include
BOOST_LIB = -lboost_system  -lboost_thread

OPENSSL_INCLUDE = -I/usr/include
OPENSSL_LIB = -lssl -lcrypto

CFLAGS = -g -O -Wall -march=x86-64 -std=c++14 $(DEFINES)\
		$(INCLUDE)\
		$(BOOST_INCLUDE)\
		$(OPENSSL_INCLUDE)

OBJS =	bit_writer.o\
		log_writer.o\
		common_function.o\
		config_parser.o\
		thread_timer.o\
		network_context_pool.o\
		network_manager.o\
		tcp_network_manager.o\
		udp_network_manager.o\
		tcp_network_object.o\
		udp_network_object.o\
		stream_manager.o\
		main_object.o\
		media_streaming_server.o\
		
		 
TARGET	= media_streaming_server

all : $(TARGET)

%.o:%.cpp
	$(CXX) -c $< -o $@ $(CFLAGS) 

$(TARGET) : $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $(OBJS) $(BASE_LIBS) $(BOOST_LIB) $(OPENSSL_LIB)

clean :
	rm -rf $(OBJS) $(TARGET) core.*
    
install :
	mkdir -p ../bin ../bin/log && touch ../bin/media_streaming_server.conf
	mv ./$(TARGET) ../bin/$(TARGET)

new : 
	$(MAKE) clean 
	$(MAKE)