#include <doctest/doctest.h>

#include <stdexcept>

#include "url_utils.hpp"

using namespace his;

TEST_CASE("parse_url splits all components") {
  const ParsedUrl p = parse_url("https://example.com:8443/path/page?id=1&t=2#frag");
  CHECK(p.scheme == "https");
  CHECK(p.authority == "example.com:8443");
  CHECK(p.path == "/path/page");
  CHECK(p.query == "id=1&t=2");
  CHECK(p.fragment == "frag");
}

TEST_CASE("parse_url handles missing query and path") {
  const ParsedUrl p = parse_url("http://host");
  CHECK(p.scheme == "http");
  CHECK(p.authority == "host");
  CHECK(p.path.empty());
  CHECK(p.query.empty());
  CHECK(p.fragment.empty());
}

TEST_CASE("validate_url accepts http/https and rejects malformed targets") {
  CHECK_NOTHROW(validate_url("https://example.com/page?id=1"));
  CHECK_NOTHROW(validate_url("HTTP://Example.com"));
  // Missing scheme: would otherwise hang until timeout.
  CHECK_THROWS_AS(validate_url("example.com/page"), std::invalid_argument);
  // Unsupported scheme.
  CHECK_THROWS_AS(validate_url("ftp://example.com"), std::invalid_argument);
  // Missing host.
  CHECK_THROWS_AS(validate_url("http://"), std::invalid_argument);
}

TEST_CASE("url_encode encodes reserved, keeps unreserved") {
  CHECK(url_encode("abcXYZ-_.~") == "abcXYZ-_.~");
  CHECK(url_encode("<a>") == "%3Ca%3E");
  CHECK(url_encode(" ") == "%20");
}

TEST_CASE("url_decode is inverse of percent encoding and handles plus") {
  CHECK(url_decode("%3Ca%3E") == "<a>");
  CHECK(url_decode("a+b") == "a b");
  // Negative: malformed escape is left as-is.
  CHECK(url_decode("%zz") == "%zz");
}

TEST_CASE("parse_query keeps order and decodes values") {
  const QueryParams q = parse_query("id=1&name=%3Cx%3E&flag");
  REQUIRE(q.size() == 3);
  CHECK(q[0].first == "id");
  CHECK(q[0].second == "1");
  CHECK(q[1].first == "name");
  CHECK(q[1].second == "<x>");
  CHECK(q[2].first == "flag");
  CHECK(q[2].second.empty());
}

TEST_CASE("parse_query on empty string yields nothing") { CHECK(parse_query("").empty()); }

TEST_CASE("extract_query_params returns unique names in order") {
  const auto names = extract_query_params("https://e.com/p?id=1&type=osago&id=2");
  REQUIRE(names.size() == 2);
  CHECK(names[0] == "id");
  CHECK(names[1] == "type");
}

TEST_CASE("extract_query_params on URL without query is empty") {
  CHECK(extract_query_params("https://e.com/p").empty());
}

TEST_CASE("build_url_with_params overrides existing param") {
  const std::string out = build_url_with_params("https://e.com/p?id=1&t=2", {"id"}, "<x>");
  CHECK(out == "https://e.com/p?id=%3Cx%3E&t=2");
}

TEST_CASE("build_url_with_params appends missing params and supports batch") {
  const std::string out = build_url_with_params("https://e.com/p?a=1", {"b", "c"}, "M");
  CHECK(out == "https://e.com/p?a=1&b=M&c=M");
}

TEST_CASE("build_url_with_params preserves fragment and path") {
  const std::string out = build_url_with_params("https://e.com/x?q=1#top", {"q"}, "v");
  CHECK(out == "https://e.com/x?q=v#top");
}
