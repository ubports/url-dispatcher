###########################
# Bad URL Exec Tool
###########################

add_definitions( -DQML_BAD_URL="${CMAKE_INSTALL_FULL_DATADIR}/url-dispatcher/bad-url.qml" )
add_executable(bad-url-exec-tool exec-tool.c)
set_target_properties(bad-url-exec-tool PROPERTIES OUTPUT_NAME "exec-tool")
target_link_libraries(bad-url-exec-tool ${GIO2_LIBRARIES} ${UBUNTU_APP_LAUNCH_LIBRARIES})

install(
  TARGETS bad-url-exec-tool
  RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_LIBEXECDIR}/ubuntu-app-launch/bad-url"
)

