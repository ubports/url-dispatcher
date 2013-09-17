
###########################
# Generated Lib
###########################

set(SERVICE_GENERATED_HEADERS
service-iface.h
)

set(SERVICE_GENERATED_SOURCES
service-iface.c
)

add_gdbus_codegen(
OUTFILES SERVICE_GENERATED_SOURCES
NAME service-iface
PREFIX com.canonical.URLDispatcher.
NAMESPACE ServiceIface
SERVICE_XML ${CMAKE_CURRENT_SOURCE_DIR}/../data/com.canonical.URLDispatcher.xml
)

add_library(service-generated STATIC ${SERVICE_GENERATED_SOURCES})

target_link_libraries(service-generated
${GLIB2_LIBRARIES}
${GOBJECT2_LIBRARIES}
${GIO2_LIBRARIES}
${JSONGLIB_LIBRARIES}
)

###########################
# Service Executable
###########################

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(service-exec dispatcher.c)

set_target_properties(service-exec PROPERTIES OUTPUT_NAME "url-dispatcher")

target_link_libraries(service-exec service-generated)

###########################
# Installation
###########################

install(
  TARGETS service-exec
  RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_LIBEXECDIR}/url-dispatcher"
)
