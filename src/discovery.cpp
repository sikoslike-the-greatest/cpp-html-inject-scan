#include "discovery.hpp"

#include <fstream>
#include <regex>
#include <stdexcept>
#include <unordered_set>

#include "url_utils.hpp"

namespace his {

namespace {

std::string trim(const std::string& s) {
  const auto begin = s.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) return std::string();
  const auto end = s.find_last_not_of(" \t\r\n");
  return s.substr(begin, end - begin + 1);
}

// Collects the first capture group of every match of `pattern` in `html`.
std::vector<std::string> match_names(const std::string& html, const std::regex& pattern) {
  std::vector<std::string> names;
  auto it = std::sregex_iterator(html.begin(), html.end(), pattern);
  const auto end = std::sregex_iterator();
  for (; it != end; ++it) {
    names.push_back((*it)[1].str());
  }
  return names;
}

}  // namespace

std::vector<std::string> params_from_url(const std::string& url) {
  return extract_query_params(url);
}

std::vector<std::string> params_from_html(const std::string& html, HtmlScanMode mode) {
  static const std::regex input_re(R"(<input[^>]+name=["']?([^"'\s>]+))",
                                   std::regex::icase | std::regex::optimize);
  static const std::regex any_re(R"(name=["']?([^"'\s>]+))",
                                 std::regex::icase | std::regex::optimize);
  const std::regex& re = (mode == HtmlScanMode::Input) ? input_re : any_re;
  return dedup_preserve_order(match_names(html, re));
}

std::vector<std::string> load_wordlist(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Wordlist not found: " + path);
  }
  std::vector<std::string> params;
  std::string line;
  while (std::getline(file, line)) {
    const std::string trimmed = trim(line);
    if (!trimmed.empty()) params.push_back(trimmed);
  }
  return params;
}

const std::vector<std::string>& default_extra_params() {
  static const std::vector<std::string> params = {"bankId",
                                                  "bankName",
                                                  "balanceSum",
                                                  "birthday",
                                                  "sex",
                                                  "objectTypeId",
                                                  "coupon",
                                                  "admitadUid",
                                                  "clickId",
                                                  "subId",
                                                  "saleChannelIsn",
                                                  "curatorIsn",
                                                  "agentIsn",
                                                  "bankStringCode",
                                                  "advertiseUid",
                                                  "utmCampaign",
                                                  "utmContent",
                                                  "utmMedium",
                                                  "utmSource",
                                                  "utmTerm",
                                                  "workleUid",
                                                  "subAgent",
                                                  "sub_id",
                                                  "click_id",
                                                  "utm_source",
                                                  "utm_medium",
                                                  "utm_campaign",
                                                  "utm_content",
                                                  "utm_term",
                                                  "ref",
                                                  "redirect",
                                                  "url",
                                                  "next",
                                                  "return",
                                                  "callback",
                                                  "search",
                                                  "q",
                                                  "query",
                                                  "page",
                                                  "id"};
  return params;
}

std::vector<std::string> dedup_preserve_order(const std::vector<std::string>& items) {
  std::vector<std::string> out;
  std::unordered_set<std::string> seen;
  for (const auto& item : items) {
    if (seen.insert(item).second) out.push_back(item);
  }
  return out;
}

}  // namespace his
