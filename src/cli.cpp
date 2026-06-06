#include "cli.hpp"

#include <charconv>
#include <stdexcept>

namespace his {

namespace {

std::vector<std::string> split_csv(const std::string& s) {
  std::vector<std::string> out;
  std::size_t start = 0;
  while (start <= s.size()) {
    std::size_t comma = s.find(',', start);
    if (comma == std::string::npos) comma = s.size();
    std::string token = s.substr(start, comma - start);
    const auto b = token.find_first_not_of(" \t");
    if (b != std::string::npos) {
      const auto e = token.find_last_not_of(" \t");
      out.push_back(token.substr(b, e - b + 1));
    }
    if (comma == s.size()) break;
    start = comma + 1;
  }
  return out;
}

template <typename T>
T parse_number(const std::string& s, const std::string& opt) {
  T value{};
  const auto result = std::from_chars(s.data(), s.data() + s.size(), value);
  if (result.ec != std::errc() || result.ptr != s.data() + s.size()) {
    throw std::invalid_argument("Invalid number for " + opt + ": " + s);
  }
  return value;
}

}  // namespace

Options parse_args(const std::vector<std::string>& args) {
  Options opt;

  auto need_value = [&](std::size_t& i, const std::string& name) -> const std::string& {
    if (i + 1 >= args.size()) {
      throw std::invalid_argument("Missing value for option " + name);
    }
    return args[++i];
  };

  for (std::size_t i = 0; i < args.size(); ++i) {
    const std::string& a = args[i];
    if (a == "-u" || a == "--url") {
      opt.url_single = need_value(i, a);
    } else if (a == "-l" || a == "--list") {
      opt.url_list_path = need_value(i, a);
    } else if (a == "-p" || a == "--payload") {
      opt.payload = need_value(i, a);
    } else if (a == "-m" || a == "--marker") {
      opt.marker = need_value(i, a);
    } else if (a == "-w" || a == "--wordlist") {
      opt.wordlist_path = need_value(i, a);
    } else if (a == "--extra") {
      opt.extra = split_csv(need_value(i, a));
    } else if (a == "--no-extra") {
      opt.use_builtin_extra = false;
    } else if (a == "--header") {
      opt.headers.push_back(need_value(i, a));
    } else if (a == "--cookie") {
      opt.cookie = need_value(i, a);
    } else if (a == "--proxy") {
      opt.proxy = need_value(i, a);
    } else if (a == "--method") {
      const std::string& m = need_value(i, a);
      if (m == "GET" || m == "get") {
        opt.method = "GET";
      } else if (m == "POST" || m == "post") {
        opt.method = "POST";
      } else {
        throw std::invalid_argument("Invalid --method (use GET or POST): " + m);
      }
    } else if (a == "--mode") {
      const std::string& md = need_value(i, a);
      if (md == "input") {
        opt.mode = HtmlScanMode::Input;
      } else if (md == "all") {
        opt.mode = HtmlScanMode::All;
      } else {
        throw std::invalid_argument("Invalid --mode (use input or all): " + md);
      }
    } else if (a == "--no-html-scan") {
      opt.no_html_scan = true;
    } else if (a == "--auto") {
      opt.autoselect = true;
    } else if (a == "-s" || a == "--silent") {
      opt.silent = true;
    } else if (a == "--max-url-len") {
      opt.max_url_len = parse_number<std::size_t>(need_value(i, a), a);
    } else if (a == "--threads") {
      opt.threads = parse_number<std::size_t>(need_value(i, a), a);
    } else if (a == "--timeout") {
      opt.timeout_ms = parse_number<long>(need_value(i, a), a);
    } else if (a == "-o" || a == "--output") {
      opt.output_path = need_value(i, a);
    } else if (a == "-h" || a == "--help") {
      opt.show_help = true;
    } else {
      throw std::invalid_argument("Unknown option: " + a);
    }
  }
  return opt;
}

std::string usage_text() {
  return
      "html-inject-scan - automated HTML injection scanner\n"
      "\n"
      "Usage:\n"
      "  html-inject-scan -u <url> [options]\n"
      "  html-inject-scan -l <urls.txt> [options]\n"
      "\n"
      "Targets:\n"
      "  -u, --url <url>         single target URL\n"
      "  -l, --list <file>       file with one URL per line\n"
      "\n"
      "Injection:\n"
      "  -p, --payload <str>     payload to inject (default: '\"><zxcasd>)\n"
      "  -m, --marker <str>      substring to look for in response (default: <zxcasd>)\n"
      "\n"
      "Parameter sources:\n"
      "  -w, --wordlist <file>   custom parameter wordlist (one per line)\n"
      "      --extra <a,b,c>     extra parameter names (comma-separated)\n"
      "      --no-extra          do not use the built-in parameter set\n"
      "      --mode <input|all>  HTML scan mode (default: input)\n"
      "      --no-html-scan      do not fetch page to discover name= attributes\n"
      "\n"
      "Session:\n"
      "      --header <line>     custom header 'Name: value' (repeatable)\n"
      "      --cookie <str>      cookie string 'k=v; k2=v2'\n"
      "      --proxy <url>       HTTP proxy, e.g. http://127.0.0.1:8080\n"
      "      --method <GET|POST> HTTP method (default: GET)\n"
      "      --timeout <ms>      request timeout in ms (default: 15000)\n"
      "\n"
      "Scan behaviour:\n"
      "      --auto              non-interactive: select all discovered params\n"
      "      --max-url-len <n>   max URL length per batch (default: 2000)\n"
      "      --threads <n>       scan up to N urls in parallel (needs --auto, default: 1)\n"
      "  -s, --silent            print only findings\n"
      "  -o, --output <file>     save findings to a TSV file\n"
      "  -h, --help              show this help\n";
}

}  // namespace his
