execute_process(COMMAND python3 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())"
    OUTPUT_VARIABLE PYTHON_PACKAGE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

install(FILES fixture_setup.py fake_dispatcher.py test_fake_dispatcher.py
    DESTINATION ${PYTHON_PACKAGE_DIR}/url_dispatcher_testability
)

configure_file(
  "${PROJECT_SOURCE_DIR}/tests/url_dispatcher_testability/test_fake_dispatcher.py.in"
  "${PROJECT_SOURCE_DIR}/tests/url_dispatcher_testability/test_fake_dispatcher.py"
  )

add_test(fake-dispatcher dbus-test-runner --task nosetests3 --parameter ${PROJECT_SOURCE_DIR}/tests/url_dispatcher_testability/test_fake_dispatcher.py)
