set(SRC_SERVER
	AbstractClientConnection.h
	AbstractClientConnection.cpp
	ConnectionManager.h
	ConnectionManager.cpp
	ObjectWithId.h
	SyncableList.h
	SyncableList.cpp
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

add_library(jd-tablesync-server ${SRC_SERVER})
target_link_libraries(jd-tablesync-server
	Qt5::Core ${EXTRA_SERVER_LIBS}
	jd-util jd-tablesync-common
)
target_include_directories(jd-tablesync-server PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<INSTALL_INTERFACE:include>
)

install(TARGETS jd-tablesync-server EXPORT jd-tablesync DESTINATION lib)
