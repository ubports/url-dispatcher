
# Google Test

include_directories(${GTEST_INCLUDE_DIR})

add_library (gtest STATIC
             ${GTEST_SOURCE_DIR}/gtest-all.cc
             ${GTEST_SOURCE_DIR}/gtest_main.cc)

# App ID URL test

add_executable (dispatcher-test dispatcher-test.cc)
target_link_libraries (dispatcher-test gtest ${GTEST_LIBS})

add_test (dispatcher-test dispatcher-test)

