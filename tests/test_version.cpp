#include <doctest/doctest.h>

#include "version.hpp"

TEST_CASE("version string contains the app name") {
  const std::string v = his::version_string();
  CHECK(v.find("html-inject-scan") != std::string::npos);
}

TEST_CASE("version string is non-empty") { CHECK_FALSE(his::version_string().empty()); }
