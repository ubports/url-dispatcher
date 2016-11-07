
install(FILES
	url-dispatcher-gui.qml
	url-dispatcher-gui.svg
	DESTINATION "${CMAKE_INSTALL_DATADIR}/url-dispatcher/gui/")
install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/url-dispatcher-gui.desktop
	DESTINATION "${CMAKE_INSTALL_DATADIR}/applications/")

configure_file(
	"url-dispatcher-gui.desktop.in"
	"url-dispatcher-gui.desktop"
	@ONLY)
