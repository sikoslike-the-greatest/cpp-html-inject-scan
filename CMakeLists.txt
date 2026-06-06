cmake_minimum_required(VERSION 3.16)

project(
  html_inject_scan
  VERSION 0.1.0
  DESCRIPTION "Automated HTML injection scanner for web applications"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)

include(FetchContent)

# Third-party HTTP client (built by CMake via FetchContent).
FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG 1.11.0)
FetchContent_MakeAvailable(cpr)

# Header-only test framework (built by CMake via FetchContent).
# doctest 2.4.11 declares a CMake minimum older than CMake 4 accepts, so we
# relax the policy minimum just for its sub-configuration.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.11)
FetchContent_MakeAvailable(doctest)
unset(CMAKE_POLICY_VERSION_MINIMUM)

# Core library: every module lives here so both the executable and the
# test target build from exactly the same sources.
add_library(
  scanner_lib
  src/version.cpp
  src/url_utils.cpp
  src/http_client.cpp
  src/discovery.cpp
  src/selector.cpp
  src/scanner.cpp
  src/cli.cpp
  src/report.cpp)
target_include_directories(scanner_lib PUBLIC src)
target_link_libraries(scanner_lib PUBLIC cpr::cpr Threads::Threads)
target_compile_definitions(scanner_lib PUBLIC HIS_VERSION="${PROJECT_VERSION}")

add_executable(html-inject-scan src/main.cpp)
target_link_libraries(html-inject-scan PRIVATE scanner_lib)

enable_testing()
add_subdirectory(tests)
