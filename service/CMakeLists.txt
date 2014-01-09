
include(UseConstantBuilder)

###########################
# Generated Lib
###########################


set(SERVICE_GENERATED_HEADERS
create-db-sql.h
service-iface.h
)

set(SERVICE_GENERATED_SOURCES
service-iface.c
)

add_constant_template(SERVICE_GENERATED_SOURCES
	create-db-sql
	create_db_sql
	"${CMAKE_CURRENT_SOURCE_DIR}/create-db.sql"
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
	url-db.c
	dispatcher.h
	dispatcher.c)

target_link_libraries(dispatcher-lib
	service-generated
	${GLIB2_LIBRARIES}
	${GOBJECT2_LIBRARIES}
	${GIO2_LIBRARIES}
	${SQLITE_LIBRARIES}
)

###########################
# Service Executable
###########################

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(service-exec service.c recoverable-problem.h recoverable-problem.c)

set_target_properties(service-exec PROPERTIES OUTPUT_NAME "url-dispatcher")

target_link_libraries(service-exec dispatcher-lib ${UPSTART_APP_LAUNCH_LIBRARIES})

###########################
# Update Directory
###########################

add_executable(update-directory update-directory.c url-db.c create-db-sql.c)
set_target_properties(update-directory PROPERTIES OUTPUT_NAME "update-directory")
target_link_libraries(update-directory ${GIO2_LIBRARIES} ${SQLITE_LIBRARIES} ${JSONGLIB_LIBRARIES})

###########################
# Installation
###########################

install(
  TARGETS service-exec update-directory
  RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_LIBEXECDIR}/url-dispatcher"
)

