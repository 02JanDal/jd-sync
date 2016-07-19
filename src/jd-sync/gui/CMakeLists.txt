set(SRC_GUI
	SyncedWidgetsGroup.h
	SyncedWidgetsGroup.cpp
)

add_library(jd-sync-gui ${SRC_GUI})
target_link_libraries(jd-sync-gui Qt5::Core Qt5::Widgets jd-sync-client)
target_include_directories(jd-sync-gui PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
	$<INSTALL_INTERFACE:include>
)

install(TARGETS jd-sync-gui EXPORT jd-sync DESTINATION lib)

cppcheck_target(jd-sync-gui)
