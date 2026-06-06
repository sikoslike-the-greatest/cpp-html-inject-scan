#include "report.hpp"

#include <fstream>
#include <ostream>
#include <stdexcept>

namespace his {

namespace {

constexpr const char* kReset = "\033[0m";
constexpr const char* kRed = "\033[91m";
constexpr const char* kGreen = "\033[92m";
constexpr const char* kYellow = "\033[93m";
constexpr const char* kCyan = "\033[96m";
constexpr const char* kDim = "\033[90m";

}  // namespace

Reporter::Reporter(std::ostream& out, bool silent, bool color)
    : out_(out), silent_(silent), color_(color) {}

const char* Reporter::paint(const char* code) const { return color_ ? code : ""; }

void Reporter::run_info(const std::string& payload, const std::string& marker,
                        const std::string& method, std::size_t url_count) const {
  if (silent_) return;
  out_ << paint(kCyan) << "html-inject-scan" << paint(kReset) << "\n";
  out_ << paint(kDim) << "  payload : " << paint(kReset) << paint(kRed) << payload << paint(kReset)
       << "\n";
  out_ << paint(kDim) << "  marker  : " << paint(kReset) << paint(kYellow) << marker
       << paint(kReset) << "\n";
  out_ << paint(kDim) << "  method  : " << paint(kReset) << method << "\n";
  out_ << paint(kDim) << "  urls    : " << paint(kReset) << url_count << "\n";
}

void Reporter::scan_header(const std::string& url, std::size_t param_count,
                           const std::string& method) const {
  if (silent_) return;
  out_ << "\n"
       << paint(kCyan) << "[*] Scanning " << url << paint(kReset) << " (" << param_count
       << " params, method=" << method << ")\n";
}

void Reporter::skipped(const std::string& url) const {
  if (silent_) return;
  out_ << paint(kDim) << "  no params selected for " << url << ", skipping" << paint(kReset)
       << "\n";
}

void Reporter::warn(const std::string& message) const {
  if (silent_) return;
  out_ << paint(kYellow) << "  [!] " << message << paint(kReset) << "\n";
}

void Reporter::hit(const ScanHit& hit) const {
  out_ << "  " << paint(kRed) << "[REFLECTED] " << hit.param << paint(kReset) << "  "
       << paint(kGreen) << hit.status << paint(kReset) << "  " << hit.url << "\n";
}

void Reporter::summary(std::size_t total_hits) const {
  if (silent_) return;
  out_ << "\n";
  if (total_hits == 0) {
    out_ << paint(kDim) << "No reflections found." << paint(kReset) << "\n";
  } else {
    out_ << paint(kRed) << "REFLECTED: " << total_hits << " hit(s)" << paint(kReset) << "\n";
  }
}

void save_hits_tsv(const std::string& path, const std::vector<ScanHit>& hits) {
  std::ofstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open output file for writing: " + path);
  }
  for (const auto& h : hits) {
    file << h.param << '\t' << h.status << '\t' << h.url << '\n';
  }
}

}  // namespace his
