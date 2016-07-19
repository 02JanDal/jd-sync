set(SRC_CLIENT
	lists/AbstractRecordList.h
	lists/AbstractRecordList.cpp
	lists/SyncedList.h
	lists/SyncedList.cpp
	lists/ChangeTrackingList.h
	lists/ChangeTrackingList.cpp
	lists/FilteredList.h
	lists/FilteredList.cpp
	lists/RecordListModel.h
	lists/RecordListModel.cpp

	TcpClientActor.h
	TcpClientActor.cpp
	Ping.h
	Ping.cpp

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

cppcheck_target(jd-sync-client)
