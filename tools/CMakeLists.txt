
###########################
# URL Dispatcher
###########################

include_directories(${CMAKE_SOURCE_DIR}/liburl-dispatcher)

add_executable(url-dispatcher url-dispatcher.c)

target_link_libraries(url-dispatcher
  dispatcher
)

install(
  TARGETS url-dispatcher
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(
  PROGRAMS url-dispatcher-dump
  DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

##########################
# Coverage
###########################
set_property(GLOBAL APPEND PROPERTY COVERAGE_TARGETS url-dispatcher)
