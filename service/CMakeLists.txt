
include(UseConstantBuilder)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_definitions( -DOVERLAY_SYSTEM_DIRECTORY="${CMAKE_INSTALL_FULL_DATADIR}/url-dispatcher/url-overlays" )

configure_file(config.h.in config.h @ONLY)

###########################
# Generated Lib
###########################

add_gdbus_codegen(
  SERVICE_GENERATED
  service-iface
  com.canonical.URLDispatcher.
  ${CMAKE_SOURCE_DIR}/data/com.canonical.URLDispatcher.xml
  NAMESPACE ServiceIface
)

add_library(service-generated STATIC ${SERVICE_GENERATED_SOURCES})

target_link_libraries(service-generated
  ${GLIB2_LIBRARIES}
  ${GOBJECT2_LIBRARIES}
  ${GIO2_LIBRARIES}
)

###########################
# Dispatcher Lib
###########################

add_library(update-directory-lib STATIC
	update-directory.h
	update-directory.c
)

target_link_libraries(update-directory-lib
	${GIO2_LIBRARIES}
	${JSONGLIB_LIBRARIES}
	url-db-lib
)

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
)

target_link_libraries(dispatcher-lib
	update-directory-lib
	url-db-lib
	service-generated
	-pthread
	${APPARMOR_LIBRARIES}
	${GLIB2_LIBRARIES}
	${GOBJECT2_LIBRARIES}
	${GIO2_LIBRARIES}
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

add_executable(service-exec service.cpp)

set_target_properties(service-exec PROPERTIES OUTPUT_NAME "url-dispatcher")

target_link_libraries(service-exec dispatcher-lib)

###########################
# Update Directory
###########################

add_executable(update-directory update-directory-exec.c)
set_target_properties(update-directory PROPERTIES OUTPUT_NAME "update-directory")
target_link_libraries(update-directory update-directory-lib)

###########################
# Coverage
###########################
set_property(
  GLOBAL
  APPEND
  PROPERTY
  COVERAGE_TARGETS dispatcher-lib url-db-lib service-exec update-directory)

###########################
# Installation
###########################

install(
  TARGETS service-exec update-directory
  RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_LIBEXECDIR}/url-dispatcher"
)

###########################
# Exec Tools
###########################

add_subdirectory(url-overlay)
add_subdirectory(bad-url)
