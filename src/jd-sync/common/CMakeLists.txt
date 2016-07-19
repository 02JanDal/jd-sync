set(SRC_COMMON
	TcpUtils.h
	TcpUtils.cpp

	Message.h
	Message.cpp
	MessageHub.h
	MessageHub.cpp
	AbstractActor.h
	AbstractActor.cpp
	AbstractThreadedActor.h
	AbstractThreadedActor.cpp
	AbstractExternalActor.h
	AbstractExternalActor.cpp
	Request.h
	Request.cpp
	RequestWaiter.h
	RequestWaiter.cpp
	Filter.h
	Filter.cpp
	CRUDMessages.h
	CRUDMessages.cpp

	3rdparty/avahi-qt/qt-watch.h
	3rdparty/avahi-qt/qt-watch.cpp
)

add_library(jd-sync-common ${SRC_COMMON})
target_link_libraries(jd-sync-common Qt5::Core Qt5::Network jd-util)
target_include_directories(jd-sync-common PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include>
)

install(TARGETS jd-sync-common EXPORT jd-sync DESTINATION lib)

cppcheck_target(jd-sync-common)
