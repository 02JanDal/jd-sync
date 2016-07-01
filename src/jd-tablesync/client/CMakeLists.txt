set(SRC_CLIENT
	AbstractConsumer.h
	AbstractConsumer.cpp
	ServerConnection.h
	ServerConnection.cpp
	SyncedList.h
	SyncedList.cpp

	MonitorModel.h
	MonitorModel.cpp

	ServiceDiscoveryModel.h
	ServiceDiscoveryModel.cpp
)

add_library(jd-tablesync-client ${SRC_CLIENT})
target_link_libraries(jd-tablesync-client Qt5::Core Qt5::Network avahi-client jd-util jd-tablesync-common)
target_include_directories(jd-tablesync-client PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<INSTALL_INTERFACE:include>
)

install(TARGETS jd-tablesync-client EXPORT jd-tablesync DESTINATION lib)
