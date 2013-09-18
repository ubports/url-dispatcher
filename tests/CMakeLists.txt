
# Google Test

include_directories(${GTEST_INCLUDE_DIR})

add_library (gtest STATIC
             ${GTEST_SOURCE_DIR}/gtest-all.cc
             ${GTEST_SOURCE_DIR}/gtest_main.cc)

# App ID URL test

add_executable (app-id-test app-id-test.cc)
target_link_libraries (app-id-test gtest ${GTEST_LIBS})

add_test (app-id-test app-id-test)

