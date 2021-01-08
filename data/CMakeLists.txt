
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
# Upstart Update Jobs
###########################

set(DISPATCHER_UPDATE_USER_UPSTART
"${CMAKE_CURRENT_BINARY_DIR}/url-dispatcher-update-user.conf"
)

configure_file("url-dispatcher-update-user.conf.in"
  "${DISPATCHER_UPDATE_USER_UPSTART}"
  @ONLY
)

install(
FILES
"${DISPATCHER_UPDATE_USER_UPSTART}"
DESTINATION "/usr/share/upstart/sessions"
)

set(DISPATCHER_UPDATE_SYSTEM_UPSTART
"${CMAKE_CURRENT_BINARY_DIR}/url-dispatcher-update-system.conf"
)

configure_file("url-dispatcher-update-system.conf.in"
  "${DISPATCHER_UPDATE_SYSTEM_UPSTART}"
  @ONLY
)

install(
FILES
"${DISPATCHER_UPDATE_SYSTEM_UPSTART}"
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

###########################
# QML
###########################

install(
FILES
bad-url.qml
DESTINATION ${datadir}/url-dispatcher
)

configure_file("url-dispatcher.service.in"
  "url-dispatcher.service"
  @ONLY
)

configure_file("com.canonical.URLDispatcher.service.in"
  "com.canonical.URLDispatcher.service"
  @ONLY
)

set (SYSTEMD_USER_DIR "/usr/lib/systemd/user")
install(
  FILES url-dispatcher.service
  DESTINATION ${SYSTEMD_USER_DIR}
)

install(FILES com.canonical.URLDispatcher.service DESTINATION
  ${CMAKE_INSTALL_DATADIR}/dbus-1/services
)
