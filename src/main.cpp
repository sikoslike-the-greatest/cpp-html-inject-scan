#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "cli.hpp"
#include "discovery.hpp"
#include "http_client.hpp"
#include "scanner.hpp"
#include "selector.hpp"
#include "url_utils.hpp"
#include "version.hpp"

/// \file main.cpp
/// \brief Точка входа: разбор аргументов и склейка модулей сканера.

namespace {

using his::HtmlScanMode;
using his::HttpResponse;
using his::Options;
using his::QueryParams;
using his::ScanHit;
using his::Session;

// Builds the request sender used by the scanner. For POST the query of the
// probe URL is moved into the request body, matching the original tool.
his::ResponseFn make_sender(const Session& session, const std::string& method) {
  return [&session, method](const std::string& url) -> HttpResponse {
    if (method == "POST") {
      his::ParsedUrl parsed = his::parse_url(url);
      const QueryParams form = his::parse_query(parsed.query);
      parsed.query.clear();
      return session.post(his::build_url(parsed), form);
    }
    return session.get(url);
  };
}

Session build_session(const Options& opt) {
  Session session;
  for (const auto& h : opt.headers) session.add_header_line(h);
  if (!opt.cookie.empty()) session.set_cookies(opt.cookie);
  if (!opt.proxy.empty()) session.set_proxy(opt.proxy);
  session.set_timeout_ms(opt.timeout_ms);
  return session;
}

std::vector<std::string> collect_urls(const Options& opt) {
  std::vector<std::string> urls;
  if (!opt.url_single.empty()) urls.push_back(opt.url_single);
  if (!opt.url_list_path.empty()) {
    for (auto& u : his::load_wordlist(opt.url_list_path)) urls.push_back(std::move(u));
  }
  return urls;
}

// Gathers candidate parameters for a URL grouped by source label.
std::vector<std::pair<std::string, std::vector<std::string>>> gather_sources(
    const Session& session, const std::string& url, const Options& opt) {
  std::vector<std::pair<std::string, std::vector<std::string>>> sources;

  const auto url_params = his::params_from_url(url);
  if (!url_params.empty()) sources.emplace_back("URL query", url_params);

  if (!opt.no_html_scan) {
    try {
      const HttpResponse page = session.get(url);
      auto html_params = his::params_from_html(page.text, opt.mode);
      if (!html_params.empty()) sources.emplace_back("HTML page", std::move(html_params));
    } catch (const std::exception& e) {
      std::cerr << "  [!] HTML scan failed: " << e.what() << "\n";
    }
  }

  if (!opt.wordlist_path.empty()) {
    sources.emplace_back("Wordlist", his::load_wordlist(opt.wordlist_path));
  }

  std::vector<std::string> extra = opt.extra;
  if (opt.use_builtin_extra) {
    const auto& builtin = his::default_extra_params();
    extra.insert(extra.end(), builtin.begin(), builtin.end());
  }
  if (!extra.empty()) sources.emplace_back("Extra/builtin", his::dedup_preserve_order(extra));

  return sources;
}

std::vector<std::string> select_params(
    const std::vector<std::pair<std::string, std::vector<std::string>>>& sources,
    const Options& opt) {
  std::vector<std::string> merged;
  for (const auto& src : sources) {
    const auto chosen =
        opt.autoselect ? src.second : his::interactive_select(src.second, src.first);
    merged.insert(merged.end(), chosen.begin(), chosen.end());
  }
  return his::dedup_preserve_order(merged);
}

void run(const Options& opt) {
  const Session session = build_session(opt);
  const auto sender = make_sender(session, opt.method);
  const std::vector<std::string> urls = collect_urls(opt);

  if (!opt.silent) {
    std::cout << his::version_string() << "\n";
    std::cout << "  payload : " << opt.payload << "\n";
    std::cout << "  marker  : " << opt.marker << "\n";
    std::cout << "  method  : " << opt.method << "\n";
    std::cout << "  urls    : " << urls.size() << "\n";
  }

  std::vector<ScanHit> all_hits;
  for (const auto& url : urls) {
    const auto sources = gather_sources(session, url, opt);
    const auto selected = select_params(sources, opt);
    if (selected.empty()) {
      if (!opt.silent) std::cout << "  no params selected for " << url << ", skipping\n";
      continue;
    }
    if (!opt.silent) {
      std::cout << "\n[*] Scanning " << url << " (" << selected.size()
                << " params, method=" << opt.method << ")\n";
    }
    const auto hits =
        his::scan_url(url, selected, opt.payload, opt.marker, opt.max_url_len, sender);
    for (const auto& h : hits) {
      std::cout << "  [REFLECTED] " << h.param << "  " << h.status << "  " << h.url << "\n";
    }
    all_hits.insert(all_hits.end(), hits.begin(), hits.end());
  }

  if (!opt.silent) {
    std::cout << "\n" << (all_hits.empty() ? "No reflections found." : "Done.") << "\n";
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const std::vector<std::string> args(argv + 1, argv + argc);
    const Options opt = his::parse_args(args);

    if (opt.show_help || (opt.url_single.empty() && opt.url_list_path.empty())) {
      std::cout << his::usage_text();
      return opt.show_help ? 0 : 1;
    }

    run(opt);
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
}
