
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
# Upstart Watcher Jobs
###########################

set(DISPATCHER_USER_WATCH_UPSTART
"${CMAKE_CURRENT_BINARY_DIR}/url-dispatcher-user-watch.conf"
)

configure_file("url-dispatcher-user-watch.conf.in"
  "${DISPATCHER_USER_WATCH_UPSTART}"
  @ONLY
)

install(
FILES
"${DISPATCHER_USER_WATCH_UPSTART}"
DESTINATION "/usr/share/upstart/sessions"
)

set(DISPATCHER_SYSTEM_WATCH_UPSTART
"${CMAKE_CURRENT_BINARY_DIR}/url-dispatcher-system-watch.conf"
)

configure_file("url-dispatcher-system-watch.conf.in"
  "${DISPATCHER_SYSTEM_WATCH_UPSTART}"
  @ONLY
)

install(
FILES
"${DISPATCHER_SYSTEM_WATCH_UPSTART}"
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

