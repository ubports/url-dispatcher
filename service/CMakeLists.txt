
include(UseConstantBuilder)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_definitions( -DOVERLAY_SYSTEM_DIRECTORY="${CMAKE_INSTALL_FULL_DATADIR}/url-dispatcher/url-overlays" )

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
# Dispatcher Lib
###########################

add_library(dispatcher-lib STATIC
	dispatcher.h
	dispatcher.c
	glib-thread.h
	glib-thread.cpp
	overlay-tracker.h
	overlay-tracker.cpp
	overlay-tracker-iface.h
	overlay-tracker-mir.h
	overlay-tracker-mir.cpp
	scope-checker.h
	scope-checker.cpp)

target_link_libraries(dispatcher-lib
	url-db-lib
	service-generated
	-pthread
	${APPARMOR_LIBRARIES}
	${GLIB2_LIBRARIES}
	${GOBJECT2_LIBRARIES}
	${GIO2_LIBRARIES}
	${SCOPES_LIBRARIES}
	${SQLITE_LIBRARIES}
	${UBUNTU_APP_LAUNCH_LIBRARIES}
)

###########################
# URL DB Lib
###########################

set(URL_DB_SOURCES
	url-db.c
	url-db.h
	create-db-sql.h
)

add_constant_template(URL_DB_SOURCES
	create-db-sql
	create_db_sql
	"${CMAKE_CURRENT_SOURCE_DIR}/create-db.sql"
)

add_library(url-db-lib STATIC
	${URL_DB_SOURCES}
)

target_link_libraries(url-db-lib
	${GLIB2_LIBRARIES}
	${SQLITE_LIBRARIES}
)

###########################
# Service Executable
###########################

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(service-exec service.c recoverable-problem.h recoverable-problem.c)

set_target_properties(service-exec PROPERTIES OUTPUT_NAME "url-dispatcher")

target_link_libraries(service-exec dispatcher-lib)

###########################
# Update Directory
###########################

add_executable(update-directory update-directory.c recoverable-problem.c)
set_target_properties(update-directory PROPERTIES OUTPUT_NAME "update-directory")
target_link_libraries(update-directory ${GIO2_LIBRARIES} ${JSONGLIB_LIBRARIES} url-db-lib)

###########################
# URL Overlay Exec Tool
###########################

add_executable(url-overlay-exec-tool url-overlay.c recoverable-problem.c)
set_target_properties(url-overlay-exec-tool PROPERTIES OUTPUT_NAME "exec-tool")
target_link_libraries(url-overlay-exec-tool ${GIO2_LIBRARIES} ${UBUNTU_APP_LAUNCH_LIBRARIES} ${CLICK_LIBRARIES})

###########################
# Installation
###########################

install(
  TARGETS service-exec update-directory
  RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_LIBEXECDIR}/url-dispatcher"
)

install(
  TARGETS url-overlay-exec-tool
  RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_LIBEXECDIR}/ubuntu-app-launch/url-overlay"
)

