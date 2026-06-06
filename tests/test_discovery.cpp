#include <doctest/doctest.h>

#include <cstdio>
#include <fstream>
#include <stdexcept>

#include "discovery.hpp"

using namespace his;

TEST_CASE("params_from_url delegates to query parsing") {
  const auto p = params_from_url("https://e.com/x?id=1&type=2");
  REQUIRE(p.size() == 2);
  CHECK(p[0] == "id");
  CHECK(p[1] == "type");
}

TEST_CASE("params_from_html Input mode matches only <input>") {
  const std::string html = "<input name=\"login\"><meta name=\"csrf\"><input name='pass'>";
  const auto p = params_from_html(html, HtmlScanMode::Input);
  REQUIRE(p.size() == 2);
  CHECK(p[0] == "login");
  CHECK(p[1] == "pass");
}

TEST_CASE("params_from_html All mode matches any tag and dedups") {
  const std::string html = "<input name=\"login\"><meta name=\"csrf\"><select name=login>";
  const auto p = params_from_html(html, HtmlScanMode::All);
  REQUIRE(p.size() == 2);
  CHECK(p[0] == "login");
  CHECK(p[1] == "csrf");
}

TEST_CASE("params_from_html returns nothing when no name attributes") {
  CHECK(params_from_html("<div class='x'>no params</div>", HtmlScanMode::All).empty());
}

TEST_CASE("default_extra_params is non-empty and contains known names") {
  const auto& p = default_extra_params();
  CHECK_FALSE(p.empty());
  bool has_id = false;
  for (const auto& name : p)
    if (name == "id") has_id = true;
  CHECK(has_id);
}

TEST_CASE("dedup_preserve_order keeps first occurrence order") {
  const auto p = dedup_preserve_order({"a", "b", "a", "c", "b"});
  REQUIRE(p.size() == 3);
  CHECK(p[0] == "a");
  CHECK(p[1] == "b");
  CHECK(p[2] == "c");
}

TEST_CASE("load_wordlist reads non-empty trimmed lines") {
  const std::string path = "tmp_wordlist_test.txt";
  {
    std::ofstream f(path);
    f << "id\n  q  \n\n\nredirect\n";
  }
  const auto p = load_wordlist(path);
  std::remove(path.c_str());
  REQUIRE(p.size() == 3);
  CHECK(p[0] == "id");
  CHECK(p[1] == "q");
  CHECK(p[2] == "redirect");
}

TEST_CASE("load_wordlist throws on missing file") {
  CHECK_THROWS_AS(load_wordlist("definitely_missing_file_12345.txt"), std::runtime_error);
}
