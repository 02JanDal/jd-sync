cmake_minimum_required(VERSION 3.1)

project(jd-sync)

option(TCP_CONNECTION "Build with TCP connection support" ON)
option(WEBSOCKET_CONNECTION "Build with WebSocket connection support" OFF)

if(NOT TARGET jd-util)
	message(STATUS "Including jd-util from jd-sync")
	add_subdirectory(../jd-util ${CMAKE_CURRENT_BINARY_DIR}/jd-util)
endif()

find_package(Qt5 REQUIRED COMPONENTS Core)
if(TCP_CONNECTION)
	find_package(Qt5 REQUIRED COMPONENTS Network)
endif()
if(WEBSOCKET_CONNECTION)
	find_package(Qt5 REQUIRED COMPONENTS WebSockets)
endif()

find_package(Avahi)
set(AVAHI_VERSION ${AVAHI_VERSION} PARENT_SCOPE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/jd-sync)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(Helpers)

add_subdirectory(src/jd-sync/common)
add_subdirectory(src/jd-sync/server)
add_subdirectory(src/jd-sync/client)
add_subdirectory(src/jd-sync/gui)

install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")
install(EXPORT jd-sync NAMESPACE jd_ DESTINATION lib/cmake)

enable_testing()
add_subdirectory(test)
