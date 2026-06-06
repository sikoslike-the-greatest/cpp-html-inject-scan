#include "url_utils.hpp"

#include <cctype>
#include <unordered_set>

namespace his {

namespace {

bool is_unreserved(unsigned char c) {
  return std::isalnum(c) != 0 || c == '-' || c == '_' || c == '.' || c == '~';
}

int hex_value(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

}  // namespace

ParsedUrl parse_url(const std::string& url) {
  ParsedUrl out;
  std::string rest = url;

  const auto scheme_end = rest.find("://");
  if (scheme_end != std::string::npos) {
    out.scheme = rest.substr(0, scheme_end);
    rest = rest.substr(scheme_end + 3);
  }

  const auto frag_pos = rest.find('#');
  if (frag_pos != std::string::npos) {
    out.fragment = rest.substr(frag_pos + 1);
    rest = rest.substr(0, frag_pos);
  }

  const auto query_pos = rest.find('?');
  if (query_pos != std::string::npos) {
    out.query = rest.substr(query_pos + 1);
    rest = rest.substr(0, query_pos);
  }

  // Authority ends at the first '/', the remainder is the path.
  const auto path_pos = rest.find('/');
  if (path_pos != std::string::npos) {
    out.authority = rest.substr(0, path_pos);
    out.path = rest.substr(path_pos);
  } else {
    out.authority = rest;
  }

  return out;
}

std::string url_encode(const std::string& value) {
  static const char* kHex = "0123456789ABCDEF";
  std::string out;
  out.reserve(value.size() * 3);
  for (const unsigned char c : value) {
    if (is_unreserved(c)) {
      out.push_back(static_cast<char>(c));
    } else {
      out.push_back('%');
      out.push_back(kHex[c >> 4]);
      out.push_back(kHex[c & 0x0F]);
    }
  }
  return out;
}

std::string url_decode(const std::string& value) {
  std::string out;
  out.reserve(value.size());
  for (std::size_t i = 0; i < value.size(); ++i) {
    const char c = value[i];
    if (c == '+') {
      out.push_back(' ');
    } else if (c == '%' && i + 2 < value.size()) {
      const int hi = hex_value(value[i + 1]);
      const int lo = hex_value(value[i + 2]);
      if (hi >= 0 && lo >= 0) {
        out.push_back(static_cast<char>((hi << 4) | lo));
        i += 2;
      } else {
        out.push_back(c);
      }
    } else {
      out.push_back(c);
    }
  }
  return out;
}

QueryParams parse_query(const std::string& query) {
  QueryParams params;
  std::size_t start = 0;
  while (start <= query.size()) {
    std::size_t amp = query.find('&', start);
    if (amp == std::string::npos) amp = query.size();
    const std::string token = query.substr(start, amp - start);
    if (!token.empty()) {
      const auto eq = token.find('=');
      if (eq == std::string::npos) {
        params.emplace_back(url_decode(token), std::string());
      } else {
        params.emplace_back(url_decode(token.substr(0, eq)), url_decode(token.substr(eq + 1)));
      }
    }
    if (amp == query.size()) break;
    start = amp + 1;
  }
  return params;
}

std::string build_query(const QueryParams& params) {
  std::string out;
  for (const auto& kv : params) {
    if (!out.empty()) out.push_back('&');
    out += url_encode(kv.first);
    out.push_back('=');
    out += url_encode(kv.second);
  }
  return out;
}

std::string build_url(const ParsedUrl& parts) {
  std::string out;
  if (!parts.scheme.empty()) out += parts.scheme + "://";
  out += parts.authority;
  out += parts.path;
  if (!parts.query.empty()) out += "?" + parts.query;
  if (!parts.fragment.empty()) out += "#" + parts.fragment;
  return out;
}

std::vector<std::string> extract_query_params(const std::string& url) {
  const ParsedUrl parsed = parse_url(url);
  const QueryParams pairs = parse_query(parsed.query);
  std::vector<std::string> names;
  std::unordered_set<std::string> seen;
  for (const auto& kv : pairs) {
    if (seen.insert(kv.first).second) names.push_back(kv.first);
  }
  return names;
}

std::string build_url_with_params(const std::string& url,
                                  const std::vector<std::string>& params,
                                  const std::string& value) {
  ParsedUrl parsed = parse_url(url);
  QueryParams pairs = parse_query(parsed.query);

  const std::unordered_set<std::string> targets(params.begin(), params.end());
  std::unordered_set<std::string> applied;

  for (auto& kv : pairs) {
    if (targets.count(kv.first) != 0) {
      kv.second = value;
      applied.insert(kv.first);
    }
  }

  // Append target params that were not already present (order from `params`).
  for (const auto& name : params) {
    if (applied.insert(name).second) {
      pairs.emplace_back(name, value);
    }
  }

  parsed.query = build_query(pairs);
  return build_url(parsed);
}

}  // namespace his
