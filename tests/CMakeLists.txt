find_package(GMock)

set(APP_ID_TEST_URI 1)
configure_file(test-config.h.in test-config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

###########################
# Mock Lib
###########################

add_library(mock-lib STATIC
	apparmor-mock.h
	apparmor-mock.c
	recoverable-problem-mock.c
	ubuntu-app-launch-mock.h
	ubuntu-app-launch-mock.c)

target_link_libraries(mock-lib
	${GLIB2_LIBRARIES}
)

###########################
# Mir Mock Lib
###########################

add_library(mir-mock-lib SHARED
	mir-mock.h
	mir-mock.cpp)

target_link_libraries(mir-mock-lib
	-pthread
	${GLIB2_LIBRARIES}
)

set_target_properties(mir-mock-lib PROPERTIES
	OUTPUT_NAME "mir-mock"
)

###########################
# Dispatcher test
###########################

include_directories("${CMAKE_SOURCE_DIR}/service")

add_executable (dispatcher-test dispatcher-test.cc)
target_link_libraries (dispatcher-test
	dispatcher-lib
	mock-lib
	gtest
	${UBUNTU_APP_LAUNCH_LIBRARIES}
	${DBUSTEST_LIBRARIES}
	${GMOCK_LIBRARIES})

add_test (dispatcher-test dispatcher-test)

###########################
# lib test
###########################

add_executable (lib-test-no-main lib-test-no-main.c)
target_link_libraries (lib-test-no-main dispatcher)

add_executable (lib-test lib-test.cc)
target_link_libraries (lib-test
	dispatcher
	gtest
	${DBUSTEST_LIBRARIES}
	${GMOCK_LIBRARIES})

add_test (lib-test lib-test)

###########################
# service test
###########################

add_executable (service-test service-test.cc)
target_link_libraries (service-test
	url-db-lib
	dispatcher
	gtest
	${DBUSTEST_LIBRARIES}
	${UBUNTU_APP_LAUNCH_LIBRARIES}
	${GMOCK_LIBRARIES})

add_test (service-test service-test)


###########################
# create db test 
###########################

add_test (NAME create-db-test
          COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/test-sql.sh" "${CMAKE_SOURCE_DIR}/service/create-db.sql")

###########################
# update directory test
###########################

add_executable (directory-update-test directory-update-test.cc)
target_link_libraries (directory-update-test
	url-db-lib
	gtest
	${GMOCK_LIBRARIES})

add_test (directory-update-test directory-update-test)

###########################
# url db test
###########################

add_executable (url-db-test url-db-test.cc)
target_link_libraries (url-db-test
	url-db-lib
	gtest
	${GMOCK_LIBRARIES})

add_test (url-db-test url-db-test)
add_subdirectory(url_dispatcher_testability)

###########################
# overlay tracker test
###########################

add_executable (overlay-tracker-test overlay-tracker-test.cpp)
target_link_libraries (overlay-tracker-test
	dispatcher-lib
	mir-mock-lib
	mock-lib
	gtest
	${GMOCK_LIBRARIES}
	${GIO2_LIBRARIES}
	${DBUSTEST_LIBRARIES})

add_test (overlay-tracker-test overlay-tracker-test)

###########################
# Coverage
###########################
set_property(
  GLOBAL
  APPEND
  PROPERTY COVERAGE_TESTS

  directory-update-test
  dispatcher-test
  exec-tool-test
  lib-test
  overlay-tracker-test
  service-test
  url-db-test
  )
