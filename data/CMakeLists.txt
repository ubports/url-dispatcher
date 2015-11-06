
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
# Click Hook
###########################

configure_file("url-dispatcher.urls.click-hook.in"
	"${CMAKE_SOURCE_DIR}/debian/url-dispatcher.urls.click-hook"
	@ONLY
)

configure_file("url-dispatcher.url-overlay.click-hook.in"
	"${CMAKE_SOURCE_DIR}/debian/url-dispatcher.url-overlay.click-hook"
	@ONLY
)

