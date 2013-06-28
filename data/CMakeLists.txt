
###########################
# Dbus Interfaces
###########################

install(
FILES
com.canonical.URLDispatcher.xml
DESTINATION ${DBUSIFACEDIR}
)

###########################
# Upstart Job
###########################

set(DISPATCHER_UPSTART
"${CMAKE_CURRENT_BINARY_DIR}/url-dispatcher.conf"
)

set(pkglibexecdir "${CMAKE_INSTALL_FULL_LIBEXECDIR}/url-dispatcher")
configure_file("url-dispatcher.conf.in"
  "${DISPATCHER_UPSTART}"
  @ONLY
)

install(
FILES
"${DISPATCHER_UPSTART}"
DESTINATION "/usr/share/upstart/sessions"
)

