execute_process(COMMAND python3 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())"
    OUTPUT_VARIABLE PYTHON_PACKAGE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

install(FILES fixture_setup.py
    DESTINATION ${PYTHON_PACKAGE_DIR}/url_dispatcher_testability
)

add_test(fake-dispatcher python3 -m testtools.run test_fake_dispatcher)
