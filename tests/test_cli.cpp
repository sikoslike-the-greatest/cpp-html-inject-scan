#include <doctest/doctest.h>

#include <stdexcept>

#include "cli.hpp"

using namespace his;

TEST_CASE("parse_args applies defaults") {
  const Options o = parse_args({"-u", "https://e.com"});
  CHECK(o.url_single == "https://e.com");
  CHECK(o.payload == "'\"><zxcasd>");
  CHECK(o.marker == "<zxcasd>");
  CHECK(o.method == "GET");
  CHECK(o.max_url_len == 2000);
  CHECK_FALSE(o.silent);
  CHECK(o.mode == HtmlScanMode::Input);
}

TEST_CASE("parse_args reads flags and values") {
  const Options o = parse_args({"-l", "urls.txt", "-s", "--max-url-len", "500", "--method", "post",
                                "--mode", "all", "--auto"});
  CHECK(o.url_list_path == "urls.txt");
  CHECK(o.silent);
  CHECK(o.max_url_len == 500);
  CHECK(o.method == "POST");
  CHECK(o.mode == HtmlScanMode::All);
  CHECK(o.autoselect);
}

TEST_CASE("parse_args collects repeatable headers and csv extra") {
  const Options o =
      parse_args({"-u", "x", "--header", "A: 1", "--header", "B: 2", "--extra", "p1, p2 ,p3"});
  REQUIRE(o.headers.size() == 2);
  CHECK(o.headers[0] == "A: 1");
  REQUIRE(o.extra.size() == 3);
  CHECK(o.extra[0] == "p1");
  CHECK(o.extra[1] == "p2");
  CHECK(o.extra[2] == "p3");
}

TEST_CASE("parse_args throws on unknown option") {
  CHECK_THROWS_AS(parse_args({"--nope"}), std::invalid_argument);
}

TEST_CASE("parse_args throws on missing value") {
  CHECK_THROWS_AS(parse_args({"-u"}), std::invalid_argument);
}

TEST_CASE("parse_args throws on invalid number and enum") {
  CHECK_THROWS_AS(parse_args({"--max-url-len", "abc"}), std::invalid_argument);
  CHECK_THROWS_AS(parse_args({"--method", "PUT"}), std::invalid_argument);
  CHECK_THROWS_AS(parse_args({"--mode", "weird"}), std::invalid_argument);
}

TEST_CASE("usage_text is non-empty and mentions the program") {
  CHECK(usage_text().find("html-inject-scan") != std::string::npos);
}
