#include <doctest/doctest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "parallel.hpp"

using namespace his;

TEST_CASE("parallel_map preserves order of results") {
  const std::vector<int> in{1, 2, 3, 4, 5};
  const auto out = parallel_map(in, [](const int& x) { return x * x; }, 4);
  CHECK(out == std::vector<int>{1, 4, 9, 16, 25});
}

TEST_CASE("parallel_map with a single thread behaves sequentially") {
  const std::vector<int> in{3, 1, 2};
  const auto out = parallel_map(in, [](const int& x) { return x + 10; }, 1);
  CHECK(out == std::vector<int>{13, 11, 12});
}

TEST_CASE("parallel_map handles more items than threads") {
  std::vector<int> in;
  for (int i = 0; i < 20; ++i) in.push_back(i);
  const auto out = parallel_map(in, [](const int& x) { return x * 2; }, 3);
  REQUIRE(out.size() == 20);
  CHECK(out[0] == 0);
  CHECK(out[19] == 38);
}

TEST_CASE("parallel_map on empty input returns empty") {
  const std::vector<int> in;
  const auto out = parallel_map(in, [](const int& x) { return x; }, 4);
  CHECK(out.empty());
}

TEST_CASE("parallel_map propagates exceptions from tasks") {
  const std::vector<int> in{1, 2, 3};
  CHECK_THROWS_AS(parallel_map(
                      in,
                      [](const int& x) -> int {
                        if (x == 2) throw std::runtime_error("boom");
                        return x;
                      },
                      2),
                  std::runtime_error);
}
