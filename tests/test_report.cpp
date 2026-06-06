#include <doctest/doctest.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "report.hpp"
#include "scanner.hpp"

using namespace his;

namespace {

ScanHit make_hit(const std::string& param, long status, const std::string& url) {
  ScanHit h;
  h.param = param;
  h.status = status;
  h.url = url;
  h.length = url.size();
  return h;
}

}  // namespace

TEST_CASE("Reporter prints a hit without colour codes when colour disabled") {
  std::ostringstream out;
  const Reporter r(out, /*silent=*/false, /*color=*/false);
  r.hit(make_hit("coupon", 200, "https://e.com/p?coupon=x"));
  const std::string s = out.str();
  CHECK(s.find("coupon") != std::string::npos);
  CHECK(s.find("200") != std::string::npos);
  CHECK(s.find("https://e.com/p?coupon=x") != std::string::npos);
  CHECK(s.find("\033[") == std::string::npos);  // no ANSI escapes
}

TEST_CASE("Reporter silent mode suppresses non-hit output but keeps hits") {
  std::ostringstream out;
  const Reporter r(out, /*silent=*/true, /*color=*/false);
  r.run_info("p", "m", "GET", 1);
  r.scan_header("https://e.com", 2, "GET");
  r.summary(0);
  CHECK(out.str().empty());  // nothing printed in silent mode

  r.hit(make_hit("id", 200, "u"));
  CHECK(out.str().find("id") != std::string::npos);  // hits still printed
}

TEST_CASE("Reporter non-silent prints run info and summary") {
  std::ostringstream out;
  const Reporter r(out, /*silent=*/false, /*color=*/false);
  r.run_info("PL", "MK", "POST", 3);
  r.summary(2);
  const std::string s = out.str();
  CHECK(s.find("PL") != std::string::npos);
  CHECK(s.find("MK") != std::string::npos);
  CHECK(s.find("REFLECTED: 2") != std::string::npos);
}

TEST_CASE("save_hits_tsv writes tab-separated rows") {
  const std::string path = "tmp_hits_test.tsv";
  const std::vector<ScanHit> hits{make_hit("q", 200, "http://a/?q=x"),
                                   make_hit("id", 302, "http://a/?id=x")};
  save_hits_tsv(path, hits);

  std::ifstream in(path);
  std::string line1;
  std::string line2;
  std::getline(in, line1);
  std::getline(in, line2);
  in.close();
  std::remove(path.c_str());

  CHECK(line1 == "q\t200\thttp://a/?q=x");
  CHECK(line2 == "id\t302\thttp://a/?id=x");
}

TEST_CASE("save_hits_tsv throws on unwritable path") {
  const std::vector<ScanHit> hits;
  CHECK_THROWS_AS(save_hits_tsv("no_such_dir_12345/out.tsv", hits), std::runtime_error);
}
