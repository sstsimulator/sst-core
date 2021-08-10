configure_file(sst-test-core-cmake.in sst-test-core)
configure_file(sst-test-elements-cmake.in sst-test-elements)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/sst-test-core
        ${CMAKE_CURRENT_BINARY_DIR}/sst-test-elements sst_test_engine_loader.py
  DESTINATION bin
  PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    OWNER_EXECUTE
    GROUP_READ
    GROUP_EXECUTE
    WORLD_READ
    WORLD_EXECUTE)

install(
  FILES test_engine.py
        sst_unittest_parameterized.py
        sst_unittest.py
        test_engine_globals.py
        test_engine_junit.py
        test_engine.py
        test_engine_support.py
        test_engine_unittest.py
        sst_unittest_support.py
  DESTINATION libexec)
