project(url-dispatcher C CXX)
cmake_minimum_required(VERSION 2.8.9)

string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_lower) # Build types should always be lowercase but sometimes they are not.

if ("${cmake_build_type_lower}" STREQUAL "debug")
  option (werror "Treat warnings as errors." FALSE)
else()
  option (werror "Treat warnings as errors." TRUE)
endif()

enable_testing ()
option (enable_lcov "Generate Coverage Reports" ON)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

set(PACKAGE ${CMAKE_PROJECT_NAME})
set(GETTEXT_PACKAGE ${CMAKE_PROJECT_NAME})

find_package(PkgConfig REQUIRED)
include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckFunctionExists)
include(UseGlibGeneration)
include(UseGdbusCodegen)
include(UseConstantBuilder)

# Workaround for libexecdir on debian
if (EXISTS "/etc/debian_version")
  set(CMAKE_INSTALL_LIBEXECDIR ${CMAKE_INSTALL_LIBDIR})
  set(CMAKE_INSTALL_FULL_LIBEXECDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# https://bugzilla.gnome.org/show_bug.cgi?id=669355
# Since gdbus-codegen does not produce warning-free code
# and we use -Werror, only enable these warnings for debug
# builds. Drop this once the issue has been fixed and landed.
if ("${cmake_build_type_lower}" STREQUAL debug)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpedantic")# -Wextra")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic -Wextra")
endif()

if (${werror})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
  set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()

pkg_check_modules(UBUNTU_APP_LAUNCH REQUIRED ubuntu-app-launch-3>=0.5)
include_directories(${UBUNTU_APP_LAUNCH_INCLUDE_DIRS})

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

pkg_check_modules(SQLITE REQUIRED sqlite3)
include_directories(${SQLITE_INCLUDE_DIRS})

pkg_check_modules(SCOPES REQUIRED libunity-scopes)
include_directories(${SCOPES_INCLUDE_DIRS})

pkg_check_modules(APPARMOR REQUIRED libapparmor)
include_directories(${APPARMOR_INCLUDE_DIRS})

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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")

add_subdirectory(data)
add_subdirectory(service)
add_subdirectory(liburl-dispatcher)
add_subdirectory(tools)
add_subdirectory(gui)
add_subdirectory(po)
add_subdirectory(tests)

# coverage reporting
find_package(CoverageReport)
get_property(COVERAGE_TARGETS GLOBAL PROPERTY COVERAGE_TARGETS)
get_property(COVERAGE_TESTS GLOBAL PROPERTY COVERAGE_TESTS)
ENABLE_COVERAGE_REPORT(
  TARGETS ${COVERAGE_TARGETS}
  TESTS ${COVERAGE_TESTS}
  FILTER /usr/include ${CMAKE_BINARY_DIR}/*
)
