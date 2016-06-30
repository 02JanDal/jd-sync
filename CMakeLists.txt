cmake_minimum_required(VERSION 3.1)

project(jd-tablesync)

if(NOT TARGET jd-util)
	add_subdirectory(../jd-util ${CMAKE_CURRENT_BINARY_DIR}/jd-util)
endif()

find_package(Qt5 REQUIRED COMPONENTS Core Network)

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

set(SRC_CLIENT
	AbstractConsumer.h
	AbstractConsumer.cpp
	ServerConnection.h
	ServerConnection.cpp
	SyncedList.h
	SyncedList.cpp

	MonitorModel.h
	MonitorModel.cpp
)
prepend_to_list(SRC_CLIENT src/jd-tablesync/client/)

add_library(jd-tablesync-client ${SRC_CLIENT})
target_link_libraries(jd-tablesync-client Qt5::Core Qt5::Network jd-util jd-tablesync-common)
target_include_directories(jd-tablesync-client PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<INSTALL_INTERFACE:include>
)

set(SRC_SERVER
	AbstractClientConnection.h
	AbstractClientConnection.cpp
	ConnectionManager.h
	ConnectionManager.cpp
	ObjectWithId.h
	SyncableList.h
	SyncableList.cpp
)
prepend_to_list(SRC_SERVER src/jd-tablesync/server/)

add_library(jd-tablesync-server ${SRC_SERVER})
target_link_libraries(jd-tablesync-server Qt5::Core jd-util jd-tablesync-common)
target_include_directories(jd-tablesync-server PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<INSTALL_INTERFACE:include>
)

install(TARGETS jd-tablesync-common jd-tablesync-client jd-tablesync-server EXPORT jd-tablesync DESTINATION lib)
install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")
install(EXPORT jd-tablesync NAMESPACE jd_ DESTINATION lib/cmake)

enable_testing()
add_subdirectory(test)
