project(url-dispatcher C CXX)
cmake_minimum_required(VERSION 2.8.9)

option (enable_tests "Build tests" ON)
option (enable_lcov "Generate Coverage Reports" ON)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

set(PACKAGE ${CMAKE_PROJECT_NAME})
set(GETTEXT_PACKAGE ${CMAKE_PROJECT_NAME})

# Trick the H10enable_coverage script into enabling coverage by including the text below:
# CMAKE_BUILD_TYPE coverage

find_package(PkgConfig REQUIRED)
include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckFunctionExists)
include(Coverage)
include(UseGlibGeneration)
include(UseGdbusCodegen)
include(UseConstantBuilder)

# Workaround for libexecdir on debian
if (EXISTS "/etc/debian_version") 
  set(CMAKE_INSTALL_LIBEXECDIR ${CMAKE_INSTALL_LIBDIR})
  set(CMAKE_INSTALL_FULL_LIBEXECDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

pkg_check_modules(UPSTART_APP_LAUNCH REQUIRED upstart-app-launch-2)
include_directories(${UPSTART_APP_LAUNCH_INCLUDE_DIRS})

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})

pkg_check_modules(GOBJECT2 REQUIRED gobject-2.0)
include_directories(${GOBJECT2_INCLUDE_DIRS})

pkg_check_modules(GIO2 REQUIRED gio-2.0)
include_directories(${GIO2_INCLUDE_DIRS})

pkg_check_modules(GIO_UNIX2 REQUIRED gio-unix-2.0)
include_directories(${GIO_UNIX2_INCLUDE_DIRS})

pkg_check_modules(JSONGLIB REQUIRED json-glib-1.0)
include_directories(${JSONGLIB_INCLUDE_DIRS})

pkg_check_modules(DBUSTEST REQUIRED dbustest-1>=14.04.0)
include_directories(${DBUSTEST_INCLUDE_DIRS})

if(${LOCAL_INSTALL})
  set(DBUSSERVICEDIR "${CMAKE_INSTALL_DATADIR}/dbus-1/services/")
else()
  EXEC_PROGRAM(${PKG_CONFIG_EXECUTABLE} ARGS dbus-1 --variable session_bus_services_dir OUTPUT_VARIABLE DBUSSERVICEDIR )
endif()
message("Installing DBus services to ${DBUSSERVICEDIR}")

if(${LOCAL_INSTALL})
  set(DBUSIFACEDIR "${CMAKE_INSTALL_DATADIR}/dbus-1/interfaces/")
else()
  EXEC_PROGRAM(${PKG_CONFIG_EXECUTABLE} ARGS dbus-1 --variable interfaces_dir OUTPUT_VARIABLE DBUSIFACEDIR )
endif()
message("Installing DBus interfaces to ${DBUSIFACEDIR}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -fPIC")

add_subdirectory(data)
add_subdirectory(service)
add_subdirectory(liburl-dispatcher)
add_subdirectory(tools)

# testing & coverage
if (${enable_tests})
  set (GTEST_SOURCE_DIR /usr/src/gtest/src)
  set (GTEST_INCLUDE_DIR ${GTEST_SOURCE_DIR}/..)
  set (GTEST_LIBS -lpthread)
  enable_testing ()
  if (${enable_lcov})
#    include(GCov)
  endif ()
  add_subdirectory(tests)
endif ()

