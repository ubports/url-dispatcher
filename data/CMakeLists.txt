
###########################
# Dbus Interfaces
###########################

install(
FILES
com.canonical.URLDispatcher.xml
DESTINATION ${DBUSIFACEDIR}
)

###########################
# Set stuff
###########################

set(pkglibexecdir "${CMAKE_INSTALL_FULL_LIBEXECDIR}/url-dispatcher")
set(datadir "${CMAKE_INSTALL_FULL_DATADIR}")

###########################
# Upstart Job
###########################

set(DISPATCHER_UPSTART
"${CMAKE_CURRENT_BINARY_DIR}/url-dispatcher.conf"
)

configure_file("url-dispatcher.conf.in"
  "${DISPATCHER_UPSTART}"
  @ONLY
)

install(
FILES
"${DISPATCHER_UPSTART}"
DESTINATION "/usr/share/upstart/sessions"
)

###########################
# Upstart Update Job
###########################

set(DISPATCHER_UPDATE_UPSTART
"${CMAKE_CURRENT_BINARY_DIR}/url-dispatcher-update.conf"
)

configure_file("url-dispatcher-update.conf.in"
  "${DISPATCHER_UPDATE_UPSTART}"
  @ONLY
)

install(
FILES
"${DISPATCHER_UPDATE_UPSTART}"
DESTINATION "/usr/share/upstart/sessions"
)

###########################
# Upstart Refresh Job
###########################

set(DISPATCHER_REFRESH_UPSTART
"${CMAKE_CURRENT_BINARY_DIR}/url-dispatcher-refresh.conf"
)

configure_file("url-dispatcher-refresh.conf.in"
  "${DISPATCHER_REFRESH_UPSTART}"
  @ONLY
)

install(
FILES
"${DISPATCHER_REFRESH_UPSTART}"
DESTINATION "/usr/share/upstart/sessions"
)

