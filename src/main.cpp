#include <unistd.h>

#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "cli.hpp"
#include "discovery.hpp"
#include "http_client.hpp"
#include "parallel.hpp"
#include "report.hpp"
#include "scanner.hpp"
#include "selector.hpp"
#include "url_utils.hpp"

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
  // Fail fast on malformed targets (e.g. missing scheme) instead of firing a
  // request into the void and waiting for the timeout.
  for (const auto& u : urls) his::validate_url(u);
  return urls;
}

// Aggregated outcome of scanning a single URL, collected so that printing can
// happen sequentially even when scans run in parallel.
struct UrlScanResult {
  std::string url;
  std::size_t param_count = 0;
  bool skipped = false;
  std::vector<ScanHit> hits;
  std::vector<std::string> warnings;
};

// Gathers candidate parameters for a URL grouped by source label.
std::vector<std::pair<std::string, std::vector<std::string>>> gather_sources(
    const Session& session, const std::string& url, const Options& opt,
    std::vector<std::string>& warnings) {
  std::vector<std::pair<std::string, std::vector<std::string>>> sources;

  const auto url_params = his::params_from_url(url);
  if (!url_params.empty()) sources.emplace_back("URL query", url_params);

  if (!opt.no_html_scan) {
    try {
      const HttpResponse page = session.get(url);
      auto html_params = his::params_from_html(page.text, opt.mode);
      if (!html_params.empty()) sources.emplace_back("HTML page", std::move(html_params));
    } catch (const std::exception& e) {
      warnings.push_back(std::string("HTML scan failed: ") + e.what());
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

// One independent unit of checking work: a single batch of parameters tied to
// the URL it belongs to. Collecting all jobs across all URLs lets a single
// bounded thread pool parallelize every check, not just whole URLs.
struct BatchJob {
  std::size_t url_index = 0;
  std::vector<std::string> batch;
};

void run(const Options& opt) {
  const Session session = build_session(opt);
  const auto sender = make_sender(session, opt.method);
  const std::vector<std::string> urls = collect_urls(opt);

  const bool color = isatty(STDOUT_FILENO) != 0;
  const his::Reporter reporter(std::cout, opt.silent, color);
  reporter.run_info(opt.payload, opt.marker, opt.method, urls.size());

  // Phase 1: discovery + parameter selection per URL. Kept sequential because
  // selection may be interactive (reads stdin).
  std::vector<UrlScanResult> results(urls.size());
  std::vector<std::vector<std::string>> selected_per_url(urls.size());
  for (std::size_t u = 0; u < urls.size(); ++u) {
    results[u].url = urls[u];
    const auto sources = gather_sources(session, urls[u], opt, results[u].warnings);
    auto selected = select_params(sources, opt);
    results[u].param_count = selected.size();
    if (selected.empty()) results[u].skipped = true;
    selected_per_url[u] = std::move(selected);
  }

  // Phase 2: flatten every URL's batches into one global job list.
  std::vector<BatchJob> jobs;
  for (std::size_t u = 0; u < urls.size(); ++u) {
    if (results[u].skipped) continue;
    auto batches =
        his::build_batches(urls[u], selected_per_url[u], opt.payload, opt.max_url_len, opt.threads);
    for (auto& b : batches) jobs.push_back({u, std::move(b)});
  }

  // Phase 3: run all checks through one bounded thread pool.
  const auto run_job = [&](const BatchJob& j) {
    return his::scan_batch(urls[j.url_index], j.batch, opt.payload, opt.marker, sender);
  };
  std::vector<std::vector<ScanHit>> job_hits;
  if (opt.threads > 1 && jobs.size() > 1) {
    job_hits = his::parallel_map(jobs, run_job, opt.threads);
  } else {
    job_hits.reserve(jobs.size());
    for (const auto& j : jobs) job_hits.push_back(run_job(j));
  }

  // Phase 4: regroup hits back to their URL, preserving parameter order.
  for (std::size_t k = 0; k < jobs.size(); ++k) {
    auto& dst = results[jobs[k].url_index].hits;
    dst.insert(dst.end(), job_hits[k].begin(), job_hits[k].end());
  }

  std::vector<ScanHit> all_hits;
  for (const auto& res : results) {
    for (const auto& w : res.warnings) reporter.warn(w);
    if (res.skipped) {
      reporter.skipped(res.url);
      continue;
    }
    reporter.scan_header(res.url, res.param_count, opt.method);
    for (const auto& h : res.hits) reporter.hit(h);
    all_hits.insert(all_hits.end(), res.hits.begin(), res.hits.end());
  }

  reporter.summary(all_hits.size());

  if (!opt.output_path.empty()) {
    his::save_hits_tsv(opt.output_path, all_hits);
    reporter.warn("saved findings to " + opt.output_path);
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
