#include <doctest/doctest.h>

#include "http_client.hpp"

using namespace his;

TEST_CASE("parse_header_line splits name and value") {
  const auto kv = parse_header_line("Authorization: Bearer tok");
  CHECK(kv.first == "Authorization");
  CHECK(kv.second == "Bearer tok");
}

TEST_CASE("parse_header_line trims whitespace") {
  const auto kv = parse_header_line("  X-Test :  value  ");
  CHECK(kv.first == "X-Test");
  CHECK(kv.second == "value");
}

TEST_CASE("parse_header_line rejects missing colon") {
  CHECK_THROWS_AS(parse_header_line("NoColonHere"), std::invalid_argument);
}

TEST_CASE("parse_header_line rejects empty name") {
  CHECK_THROWS_AS(parse_header_line(": value"), std::invalid_argument);
}

TEST_CASE("parse_cookie_string splits pairs") {
  const auto cookies = parse_cookie_string("PHPSESSID=abc; BX_USER_ID=xyz");
  REQUIRE(cookies.size() == 2);
  CHECK(cookies[0].first == "PHPSESSID");
  CHECK(cookies[0].second == "abc");
  CHECK(cookies[1].first == "BX_USER_ID");
  CHECK(cookies[1].second == "xyz");
}

TEST_CASE("parse_cookie_string on empty string yields nothing") {
  CHECK(parse_cookie_string("").empty());
}

TEST_CASE("parse_cookie_string rejects pair without equals") {
  CHECK_THROWS_AS(parse_cookie_string("badpair"), std::invalid_argument);
}

TEST_CASE("HttpSession defaults match Python scanner") {
  HttpSession session;
  CHECK(session.user_agent() == kDefaultUserAgent);
  CHECK_FALSE(session.verify_ssl());
  CHECK(session.timeout_sec() == kDefaultTimeoutSec);
  CHECK(session.headers().empty());
  CHECK(session.cookies().empty());
  CHECK(session.proxy().empty());
}

TEST_CASE("build_session applies headers cookie and proxy") {
  const HttpSession session = build_session(
      {"Authorization: Bearer x", "X-Custom: 1"}, "SID=1; UID=2", "http://127.0.0.1:8080");
  REQUIRE(session.headers().size() == 2);
  CHECK(session.headers()[0].first == "Authorization");
  CHECK(session.headers()[1].second == "1");
  REQUIRE(session.cookies().size() == 2);
  CHECK(session.cookies()[0].first == "SID");
  CHECK(session.proxy() == "http://127.0.0.1:8080");
}

TEST_CASE("set_timeout rejects non-positive values") {
  HttpSession session;
  CHECK_THROWS_AS(session.set_timeout(0), std::invalid_argument);
  CHECK_THROWS_AS(session.set_timeout(-1), std::invalid_argument);
}

TEST_CASE("HttpSession GET with malformed url throws") {
  HttpSession session;
  CHECK_THROWS_AS(session.get("not-a-url"), std::runtime_error);
}

TEST_CASE("HttpSession POST with malformed url throws") {
  HttpSession session;
  const QueryParams form = {{"id", "1"}};
  CHECK_THROWS_AS(session.post("not-a-url", form), std::runtime_error);
}
