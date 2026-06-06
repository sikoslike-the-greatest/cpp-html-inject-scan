add_executable(
  unit_tests
  test_main.cpp
  test_version.cpp
  test_url_utils.cpp
  test_http_client.cpp
  test_discovery.cpp
  test_selector.cpp
  test_scanner.cpp
  test_cli.cpp)
target_link_libraries(unit_tests PRIVATE scanner_lib doctest::doctest)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(unit_tests)
