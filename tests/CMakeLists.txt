
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
	upstart-app-launch-mock.c)

target_link_libraries(mock-lib
	${GLIB2_LIBRARIES}
)

###########################
# App ID URL test
###########################

add_executable (dispatcher-test dispatcher-test.cc)
target_link_libraries (dispatcher-test
	dispatcher-lib
	mock-lib
	gtest
	${GTEST_LIBS})

add_test (dispatcher-test dispatcher-test)

