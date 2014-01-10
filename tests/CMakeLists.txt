
add_definitions ( -DCMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}" )

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
	recoverable-problem-mock.c
	upstart-app-launch-mock.h
	upstart-app-launch-mock.c)

target_link_libraries(mock-lib
	${GLIB2_LIBRARIES}
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
	${GTEST_LIBS})

add_test (dispatcher-test dispatcher-test)

###########################
# App ID URL test
###########################

include_directories("${CMAKE_BINARY_DIR}/service")

add_definitions ( -DCMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" )
add_definitions ( -DAPP_ID_TEST_URI=1 )

add_executable (app-id-test app-id-test.cc "${CMAKE_SOURCE_DIR}/service/dispatcher.c")
target_link_libraries (app-id-test
	dispatcher-lib
	mock-lib
	gtest
	${GTEST_LIBS})

add_test (app-id-test app-id-test)

###########################
# lib test
###########################

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

add_definitions ( -DURL_DISPATCHER_SERVICE="${CMAKE_BINARY_DIR}/service/url-dispatcher" )

add_executable (service-test service-test.cc)
target_link_libraries (service-test
	dispatcher
	gtest
	${DBUSTEST_LIBRARIES}
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

add_definitions ( -DUPDATE_DIRECTORY_TOOL="${CMAKE_BINARY_DIR}/service/update-directory" )
add_definitions ( -DUPDATE_DIRECTORY_URLS="${CMAKE_CURRENT_SOURCE_DIR}/test-urls-simple" )

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

