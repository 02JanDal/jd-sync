cmake_minimum_required(VERSION 3.1)

project(jd-tablesync)

option(TCP_CONNECTION "Build with TCP connection support" ON)
option(WEBSOCKET_CONNECTION "Build with WebSocket connection support" OFF)

if(NOT TARGET jd-util)
	add_subdirectory(../jd-util ${CMAKE_CURRENT_BINARY_DIR}/jd-util)
endif()

find_package(Qt5 REQUIRED COMPONENTS Core)
if(TCP_CONNECTION)
	find_package(Qt5 REQUIRED COMPONENTS Network)
endif()
if(WEBSOCKET_CONNECTION)
	find_package(Qt5 REQUIRED COMPONENTS WebSockets)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(Helpers)

set(SRC_COMMON
	TcpUtils.h
	TcpUtils.cpp
)
prepend_to_list(SRC_COMMON src/jd-tablesync/common/)

add_library(jd-tablesync-common ${SRC_COMMON})
target_link_libraries(jd-tablesync-common Qt5::Core Qt5::Network jd-util)
target_include_directories(jd-tablesync-common PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<INSTALL_INTERFACE:include>
)

add_subdirectory(src/jd-tablesync/client)
add_subdirectory(src/jd-tablesync/server)

install(TARGETS jd-tablesync-common EXPORT jd-tablesync DESTINATION lib)
install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")
install(EXPORT jd-tablesync NAMESPACE jd_ DESTINATION lib/cmake)

enable_testing()
add_subdirectory(test)
