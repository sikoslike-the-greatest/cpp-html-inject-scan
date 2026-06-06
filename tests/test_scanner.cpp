#include <doctest/doctest.h>

#include <algorithm>
#include <set>
#include <string>

#include "scanner.hpp"
#include "url_utils.hpp"

using namespace his;

namespace {

const std::string kPayload = "'\"><zxcasd>";
const std::string kMarker = "<zxcasd>";

// Fake server: reflects the marker only if a vulnerable parameter carries it.
ResponseFn make_server(std::set<std::string> vulnerable) {
  return [vulnerable](const std::string& url) -> HttpResponse {
    const ParsedUrl parsed = parse_url(url);
    bool reflect = false;
    for (const auto& kv : parse_query(parsed.query)) {
      if (vulnerable.count(kv.first) != 0 && kv.second.find(kMarker) != std::string::npos) {
        reflect = true;
      }
    }
    const std::string body = reflect ? "<html><zxcasd></html>" : "<html>clean</html>";
    HttpResponse r;
    r.status = 200;
    r.text = body;
    r.length = body.size();
    return r;
  };
}

bool contains(const std::vector<ScanHit>& hits, const std::string& name) {
  return std::any_of(hits.begin(), hits.end(), [&](const ScanHit& h) { return h.param == name; });
}

}  // namespace

TEST_CASE("is_reflected finds marker, negative when absent or empty") {
  CHECK(is_reflected("abc<zxcasd>def", "<zxcasd>"));
  CHECK_FALSE(is_reflected("abc def", "<zxcasd>"));
  CHECK_FALSE(is_reflected("abc", ""));
}

TEST_CASE("probe reports reflection for a vulnerable parameter") {
  const auto server = make_server({"q"});
  const ProbeResult yes = probe("https://e.com/p", {"q"}, kPayload, kMarker, server);
  CHECK(yes.reflected);
  const ProbeResult no = probe("https://e.com/p", {"safe"}, kPayload, kMarker, server);
  CHECK_FALSE(no.reflected);
}

TEST_CASE("pack_batch fills up to the URL length limit") {
  const std::vector<std::string> params{"aaaa", "bbbb", "cccc", "dddd"};
  // Large limit: everything fits in one batch.
  CHECK(pack_batch("https://e.com/p", params, 0, kPayload, 4000) == 4);
  // Tiny limit: only one parameter per batch.
  CHECK(pack_batch("https://e.com/p", params, 0, kPayload, 1) == 1);
}

TEST_CASE("scan_url finds a single vulnerable param in a big batch") {
  const std::vector<std::string> params{"a", "b", "coupon", "d", "e", "f", "g"};
  const auto hits =
      scan_url("https://e.com/p", params, kPayload, kMarker, 4000, make_server({"coupon"}));
  REQUIRE(hits.size() == 1);
  CHECK(hits[0].param == "coupon");
  CHECK(hits[0].status == 200);
}

TEST_CASE("scan_url finds multiple vulnerable params via bisect") {
  const std::vector<std::string> params{"a", "b", "c", "d", "e", "f", "g", "h"};
  const auto hits =
      scan_url("https://e.com/p", params, kPayload, kMarker, 4000, make_server({"b", "g"}));
  REQUIRE(hits.size() == 2);
  CHECK(contains(hits, "b"));
  CHECK(contains(hits, "g"));
}

TEST_CASE("scan_url returns nothing when no param reflects") {
  const std::vector<std::string> params{"a", "b", "c", "d"};
  const auto hits = scan_url("https://e.com/p", params, kPayload, kMarker, 4000, make_server({}));
  CHECK(hits.empty());
}

TEST_CASE("scan_url works across multiple batches with a small limit") {
  const std::vector<std::string> params{"a", "b", "coupon", "d", "e"};
  // Small limit forces one param per batch (linear scan path).
  const auto hits =
      scan_url("https://e.com/p", params, kPayload, kMarker, 1, make_server({"coupon"}));
  REQUIRE(hits.size() == 1);
  CHECK(hits[0].param == "coupon");
}

TEST_CASE("build_batches splits work across threads but caps URL length") {
  const std::vector<std::string> params{"a", "b", "c", "d", "e", "f", "g", "h"};
  // One thread: a single large batch (fewest requests).
  CHECK(build_batches("https://e.com/p", params, kPayload, 4000, 1).size() == 1);
  // Four threads: roughly four batches so every thread gets work.
  CHECK(build_batches("https://e.com/p", params, kPayload, 4000, 4).size() == 4);
}

TEST_CASE("parallel scan_url matches sequential results") {
  const std::vector<std::string> params{"a", "b", "c", "d", "e", "f", "g", "h"};
  const auto server = make_server({"b", "g"});
  const auto seq = scan_url("https://e.com/p", params, kPayload, kMarker, 4000, server, 1);
  const auto par = scan_url("https://e.com/p", params, kPayload, kMarker, 4000, server, 4);
  REQUIRE(par.size() == seq.size());
  CHECK(contains(par, "b"));
  CHECK(contains(par, "g"));
}
