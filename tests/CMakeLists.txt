set(APP_ID_TEST_URI 1)
configure_file(test-config.h.in test-config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

###########################
# Click Database
###########################

configure_file ("click-db/test.conf.in" "${CMAKE_CURRENT_BINARY_DIR}/click-db/test.conf" @ONLY)
set_directory_properties (PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_CURRENT_BINARY_DIR}/click-db/test.conf")

###########################
# Google Test
###########################

include_directories(${GTEST_INCLUDE_DIR})

add_library (gtest STATIC
             ${GTEST_SOURCE_DIR}/gtest-all.cc
             ${GTEST_SOURCE_DIR}/gtest_main.cc)


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
	${GTEST_LIBS})

add_test (dispatcher-test dispatcher-test)

###########################
# App ID URL test
###########################

include_directories("${CMAKE_BINARY_DIR}/service")

add_executable (app-id-test app-id-test.cc)
target_link_libraries (app-id-test
	dispatcher-lib
	mock-lib
	gtest
	${UBUNTU_APP_LAUNCH_LIBRARIES}
	${GTEST_LIBS})

add_test (app-id-test app-id-test)

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
	${GTEST_LIBS})

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
	${GTEST_LIBS})

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
	${GTEST_LIBS})

add_test (directory-update-test directory-update-test)

###########################
# url db test
###########################

add_executable (url-db-test url-db-test.cc)
target_link_libraries (url-db-test
	url-db-lib
	gtest
	${GTEST_LIBS})

add_test (url-db-test url-db-test)
add_subdirectory(url_dispatcher_testability)

###########################
# exec tool test
###########################

add_executable (exec-tool-test exec-tool-test.cc)
target_link_libraries (exec-tool-test
	gtest
	${GTEST_LIBS}
	${GIO2_LIBRARIES}
	${DBUSTEST_LIBRARIES})

add_test (exec-tool-test exec-tool-test)

###########################
# overlay tracker test
###########################

add_executable (overlay-tracker-test overlay-tracker-test.cpp)
target_link_libraries (overlay-tracker-test
	dispatcher-lib
	mir-mock-lib
	mock-lib
	gtest
	${GTEST_LIBS}
	${GIO2_LIBRARIES}
	${DBUSTEST_LIBRARIES})

add_test (overlay-tracker-test overlay-tracker-test)
