set(SRC_CLIENT
	SyncedList.h
	SyncedList.cpp
	TcpClientActor.h
	TcpClientActor.cpp

	MonitorModel.h
	MonitorModel.cpp

	ServiceDiscoveryModel.h
	ServiceDiscoveryModel.cpp
)

add_library(jd-sync-client ${SRC_CLIENT})
target_link_libraries(jd-sync-client Qt5::Core Qt5::Network avahi-client jd-util jd-db-schema jd-sync-common)
target_include_directories(jd-sync-client PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
	$<INSTALL_INTERFACE:include>
)

install(TARGETS jd-sync-client EXPORT jd-sync DESTINATION lib)
