#include <doctest/doctest.h>

#include <sstream>

#include "selector.hpp"

using namespace his;

TEST_CASE("parse_selection 'a' and empty select all") {
  CHECK(parse_selection("a", 3) == std::vector<std::size_t>{0, 1, 2});
  CHECK(parse_selection("", 3) == std::vector<std::size_t>{0, 1, 2});
  CHECK(parse_selection("  A ", 2) == std::vector<std::size_t>{0, 1});
}

TEST_CASE("parse_selection 'n' selects none") { CHECK(parse_selection("n", 5).empty()); }

TEST_CASE("parse_selection handles single indices and ranges") {
  CHECK(parse_selection("0,2", 5) == std::vector<std::size_t>{0, 2});
  CHECK(parse_selection("1-3", 5) == std::vector<std::size_t>{1, 2, 3});
  CHECK(parse_selection("3,0-1", 5) == std::vector<std::size_t>{0, 1, 3});
}

TEST_CASE("parse_selection ignores out-of-range and invalid tokens") {
  CHECK(parse_selection("99", 3).empty());
  CHECK(parse_selection("x,1,foo", 3) == std::vector<std::size_t>{1});
  CHECK(parse_selection("2-100", 4) == std::vector<std::size_t>{2, 3});
}

TEST_CASE("parse_selection deduplicates overlapping selections") {
  CHECK(parse_selection("1,1-2,2", 5) == std::vector<std::size_t>{1, 2});
}

TEST_CASE("select_by_indices maps indices to items and skips invalid") {
  const std::vector<std::string> items{"a", "b", "c"};
  CHECK(select_by_indices(items, {0, 2}) == std::vector<std::string>{"a", "c"});
  CHECK(select_by_indices(items, {5}).empty());
}

TEST_CASE("interactive_select returns picked params from stream") {
  const std::vector<std::string> params{"id", "type", "ref"};
  std::istringstream in("0,2");
  std::ostringstream out;
  const auto picked = interactive_select(params, "URL query", in, out);
  REQUIRE(picked.size() == 2);
  CHECK(picked[0] == "id");
  CHECK(picked[1] == "ref");
}

TEST_CASE("interactive_select on empty params returns empty without reading") {
  std::istringstream in("a");
  std::ostringstream out;
  CHECK(interactive_select({}, "src", in, out).empty());
}
