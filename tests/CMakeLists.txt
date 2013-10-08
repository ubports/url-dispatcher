
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

add_definitions ( -DCMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" )

add_executable (app-id-test app-id-test.cc)
target_link_libraries (app-id-test
	dispatcher-lib
	mock-lib
	gtest
	${GTEST_LIBS})

add_test (app-id-test app-id-test)
