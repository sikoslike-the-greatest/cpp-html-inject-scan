#include "http_client.hpp"

#include <cpr/cpr.h>

#include <chrono>
#include <stdexcept>

namespace his {

namespace {

std::string trim(const std::string& s) {
  const auto begin = s.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) return std::string();
  const auto end = s.find_last_not_of(" \t\r\n");
  return s.substr(begin, end - begin + 1);
}

const char* kDefaultUserAgent =
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36";

// Applies the shared configuration onto a cpr::Session. cpr::Session is
// non-movable, so it is configured in place rather than returned by value.
void configure_session(cpr::Session& session, const std::string& url, const std::string& user_agent,
                       const std::vector<std::pair<std::string, std::string>>& headers,
                       const std::vector<std::pair<std::string, std::string>>& cookies,
                       const std::string& proxy, long timeout_ms, bool verify_ssl) {
  session.SetUrl(cpr::Url{url});
  session.SetUserAgent(cpr::UserAgent{user_agent});

  cpr::Header header_map;
  for (const auto& kv : headers) header_map[kv.first] = kv.second;
  session.SetHeader(header_map);

  if (!cookies.empty()) {
    cpr::Cookies cookie_jar;
    for (const auto& kv : cookies) cookie_jar.emplace_back(cpr::Cookie{kv.first, kv.second});
    session.SetCookies(cookie_jar);
  }

  if (!proxy.empty()) {
    session.SetProxies(cpr::Proxies{{"http", proxy}, {"https", proxy}});
  }

  session.SetTimeout(cpr::Timeout{std::chrono::milliseconds(timeout_ms)});
  session.SetVerifySsl(cpr::VerifySsl{verify_ssl});
}

HttpResponse to_response(const cpr::Response& r) {
  if (r.error) {
    throw std::runtime_error("HTTP request failed: " + r.error.message);
  }
  HttpResponse out;
  out.status = r.status_code;
  out.text = r.text;
  out.length = r.text.size();
  return out;
}

}  // namespace

std::pair<std::string, std::string> parse_header_line(const std::string& line) {
  const auto pos = line.find(':');
  if (pos == std::string::npos) {
    throw std::invalid_argument("Invalid header line (no ':'): " + line);
  }
  return {trim(line.substr(0, pos)), trim(line.substr(pos + 1))};
}

std::vector<std::pair<std::string, std::string>> parse_cookie_string(const std::string& cookies) {
  std::vector<std::pair<std::string, std::string>> out;
  std::size_t start = 0;
  while (start <= cookies.size()) {
    std::size_t sep = cookies.find(';', start);
    if (sep == std::string::npos) sep = cookies.size();
    const std::string token = trim(cookies.substr(start, sep - start));
    if (!token.empty()) {
      const auto eq = token.find('=');
      if (eq == std::string::npos) {
        out.emplace_back(token, std::string());
      } else {
        out.emplace_back(trim(token.substr(0, eq)), trim(token.substr(eq + 1)));
      }
    }
    if (sep == cookies.size()) break;
    start = sep + 1;
  }
  return out;
}

Session::Session() : user_agent_(kDefaultUserAgent) {}

void Session::set_user_agent(const std::string& ua) { user_agent_ = ua; }

void Session::add_header(const std::string& name, const std::string& value) {
  headers_.emplace_back(name, value);
}

void Session::add_header_line(const std::string& line) {
  headers_.push_back(parse_header_line(line));
}

void Session::set_cookies(const std::string& cookie_str) {
  for (auto& kv : parse_cookie_string(cookie_str)) cookies_.push_back(std::move(kv));
}

void Session::set_proxy(const std::string& proxy) { proxy_ = proxy; }

void Session::set_timeout_ms(long ms) { timeout_ms_ = ms; }

void Session::set_verify_ssl(bool verify) { verify_ssl_ = verify; }

HttpResponse Session::get(const std::string& url) const {
  cpr::Session session;
  configure_session(session, url, user_agent_, headers_, cookies_, proxy_, timeout_ms_,
                    verify_ssl_);
  return to_response(session.Get());
}

HttpResponse Session::post(const std::string& url, const QueryParams& form) const {
  cpr::Session session;
  configure_session(session, url, user_agent_, headers_, cookies_, proxy_, timeout_ms_,
                    verify_ssl_);
  cpr::Payload payload{};
  for (const auto& kv : form) payload.Add(cpr::Pair{kv.first, kv.second});
  session.SetPayload(payload);
  return to_response(session.Post());
}

}  // namespace his
