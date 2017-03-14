
###########################
# Version Information
###########################

set(API_VERSION 1)
set(ABI_VERSION 1)

###########################
# Generated Lib
###########################

set(GENERATED_HEADERS
service-iface.h
)

set(GENERATED_SOURCES
service-iface.c
)

add_gdbus_codegen(
OUTFILES GENERATED_SOURCES
NAME service-iface
PREFIX com.canonical.URLDispatcher.
NAMESPACE ServiceIface
SERVICE_XML ${CMAKE_CURRENT_SOURCE_DIR}/../data/com.canonical.URLDispatcher.xml
)

add_library(generated STATIC ${GENERATED_SOURCES})

target_link_libraries(generated
${GLIB2_LIBRARIES}
${GOBJECT2_LIBRARIES}
${GIO2_LIBRARIES}
)

###########################
# Lib
###########################

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(DISPATCHER_HEADERS
url-dispatcher.h
)

set(DISPATCHER_SOURCES
url-dispatcher.c
)

add_library(dispatcher SHARED ${DISPATCHER_SOURCES})

set_target_properties(dispatcher PROPERTIES
  VERSION ${API_VERSION}.0.0
  SOVERSION ${ABI_VERSION}
  OUTPUT_NAME "url-dispatcher"
)

target_link_libraries(dispatcher
generated
${GLIB2_LIBRARIES}
${GOBJECT2_LIBRARIES}
-Wl,--no-undefined
)

###########################
# Coverage
###########################
set_property(GLOBAL APPEND PROPERTY COVERAGE_TARGETS dispatcher)

###########################
# Pkg Config
###########################

set(DISPATCHER_PC "url-dispatcher-${API_VERSION}.pc")
set(apiversion "${API_VERSION}")
set(libdir "${CMAKE_INSTALL_FULL_LIBDIR}")
set(includedir "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
set(VERSION "${ABI_VERSION}")

configure_file("url-dispatcher.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/${DISPATCHER_PC}" @ONLY)

###########################
# Installation
###########################

install(
  FILES ${DISPATCHER_HEADERS}
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/liburl-dispatcher-${API_VERSION}"
)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${DISPATCHER_PC}"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)

install(
  TARGETS dispatcher
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
