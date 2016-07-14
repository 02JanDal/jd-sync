include(TestHelpers)

find_package(Catch REQUIRED)

add_custom_target(_dummy_for_ide_sources_ SOURCES common/DummyActor.h)

set(JDUTIL_TEST_DIR common)
set(JDUTIL_TEST_LIBS jd-sync-common)
add_unit_test(Message)
add_unit_test(MessageHubActor)
add_unit_test(ThreadedActor)
add_unit_test(Request)

add_coverage_capture(jd-sync MessageHubActor ThreadedActor Request)
