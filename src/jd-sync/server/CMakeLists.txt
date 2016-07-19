set(SRC_SERVER
	ObjectWithId.h
	SyncableList.h
	SyncableList.cpp
	RequestServer.h
	RequestServer.cpp

	ServiceAnnouncer.h
	ServiceAnnouncer.cpp
)

set(EXTRA_SERVER_LIBS )
if(TCP_CONNECTION)
	list(APPEND SRC_SERVER
		tcp/TcpServer.h tcp/TcpServer.cpp
		tcp/TcpClientConnection.h tcp/TcpClientConnection.cpp
	)
	list(APPEND EXTRA_SERVER_LIBS Qt5::Network)
endif()
if(WEBSOCKET_CONNECTION)
	list(APPEND SRC_SERVER
		websocket/WebSocketServer.h websocket/WebSocketServer.cpp
		websocket/WebSocketClientConnection.h websocket/WebSocketClientConnection.cpp
	)
list(APPEND EXTRA_SERVER_LIBS Qt5::WebSockets)
endif()

add_library(jd-sync-server ${SRC_SERVER})
target_link_libraries(jd-sync-server
	Qt5::Core ${EXTRA_SERVER_LIBS}
	jd-util jd-sync-common
	avahi-common avahi-client
)
target_include_directories(jd-sync-server PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
	$<INSTALL_INTERFACE:include>
)

install(TARGETS jd-sync-server EXPORT jd-sync DESTINATION lib)

cppcheck_target(jd-sync-server)
