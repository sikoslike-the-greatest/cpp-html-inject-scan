#include <doctest/doctest.h>

#include <stdexcept>

#include "http_client.hpp"

using namespace his;

TEST_CASE("parse_header_line splits name and value and trims") {
  const auto h = parse_header_line("Authorization:  Bearer tok123 ");
  CHECK(h.first == "Authorization");
  CHECK(h.second == "Bearer tok123");
}

TEST_CASE("parse_header_line keeps colons inside the value") {
  const auto h = parse_header_line("X-Time: 10:30:00");
  CHECK(h.first == "X-Time");
  CHECK(h.second == "10:30:00");
}

TEST_CASE("parse_header_line throws when colon is missing") {
  CHECK_THROWS_AS(parse_header_line("NoColonHere"), std::invalid_argument);
}

TEST_CASE("parse_cookie_string parses multiple pairs") {
  const auto c = parse_cookie_string("PHPSESSID=abc; BX_USER_ID=xyz");
  REQUIRE(c.size() == 2);
  CHECK(c[0].first == "PHPSESSID");
  CHECK(c[0].second == "abc");
  CHECK(c[1].first == "BX_USER_ID");
  CHECK(c[1].second == "xyz");
}

TEST_CASE("parse_cookie_string skips empty segments") {
  const auto c = parse_cookie_string("a=1;; ;b=2;");
  REQUIRE(c.size() == 2);
  CHECK(c[0].first == "a");
  CHECK(c[1].first == "b");
}

TEST_CASE("parse_cookie_string on empty input yields nothing") {
  CHECK(parse_cookie_string("").empty());
}

TEST_CASE("Session is constructible and configurable without network") {
  Session s;
  s.set_user_agent("UA/1.0");
  s.add_header("X-Test", "1");
  s.add_header_line("X-From-Line: yes");
  s.set_cookies("k=v");
  s.set_proxy("http://127.0.0.1:8080");
  s.set_timeout_ms(5000);
  s.set_verify_ssl(true);
  CHECK(true);  // No throw during configuration.
}
