cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 14)

set(LIB_SOURCES
	rpc-protocol.cpp
	rpc-socket-transport.cpp
	rpc-client.cpp
	rpc-server.cpp)
set(LIB_HEADERS
	rpc-protocol.h
	rpc-socket-transport.h
	rpc-client.h
	rpc-server.h)

add_library(rpc-common STATIC ${LIB_SOURCES} ${LIB_HEADERS})

set(CLIENT_SOURCES
	rpc-client-main.cpp)
set(CLIENT_LIBS
	rpc-common)

add_executable(rpc-client ${CLIENT_SOURCES})
target_link_libraries(rpc-client ${CLIENT_LIBS})

set(SERVER_SOURCES
	rpc-server-main.cpp)
set(SERVER_LIBS
	rpc-common)

add_executable(rpc-server ${SERVER_SOURCES})
target_link_libraries(rpc-server ${SERVER_LIBS})

set(TEST_SOURCES
	rpc-test.cpp)
set(TEST_LIBS
	rpc-common)

add_executable(rpc-test ${TEST_SOURCES})
target_link_libraries(rpc-test ${TEST_LIBS})
