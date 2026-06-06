#include "http_client.hpp"

#include <cpr/cpr.h>
#include <stdexcept>

namespace his {

namespace {

std::string trim(const std::string& s) {
  const auto start = s.find_first_not_of(" \t");
  if (start == std::string::npos) return {};
  const auto end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

cpr::Cookies to_cpr_cookies(const std::vector<std::pair<std::string, std::string>>& cookies) {
  cpr::Cookies result;
  for (const auto& kv : cookies) {
    result.push_back(cpr::Cookie{kv.first, kv.second});
  }
  return result;
}

cpr::Payload to_cpr_payload(const QueryParams& form) {
  std::vector<cpr::Pair> pairs;
  pairs.reserve(form.size());
  for (const auto& kv : form) {
    pairs.emplace_back(kv.first, kv.second);
  }
  return cpr::Payload(pairs.begin(), pairs.end());
}

HttpResponse from_cpr_response(const cpr::Response& resp) {
  if (resp.error.code != cpr::ErrorCode::OK) {
    throw std::runtime_error(resp.error.message);
  }
  HttpResponse out;
  out.status_code = static_cast<int>(resp.status_code);
  out.body = resp.text;
  return out;
}

void configure_cpr_session(cpr::Session& session, const HttpSession& cfg) {
  session.SetUserAgent(cpr::UserAgent{cfg.user_agent()});
  session.SetVerifySsl(cpr::VerifySsl{cfg.verify_ssl()});
  session.SetRedirect(cpr::Redirect{true});
  session.SetTimeout(cpr::Timeout{cfg.timeout_sec() * 1000});

  cpr::Header header;
  for (const auto& kv : cfg.headers()) {
    header[kv.first] = kv.second;
  }
  if (!header.empty()) {
    session.SetHeader(header);
  }

  if (!cfg.cookies().empty()) {
    session.SetCookies(to_cpr_cookies(cfg.cookies()));
  }

  if (!cfg.proxy().empty()) {
    session.SetProxies(
        cpr::Proxies{{"http", cfg.proxy()}, {"https", cfg.proxy()}});
  }
}

}  // namespace

std::pair<std::string, std::string> parse_header_line(const std::string& line) {
  const auto colon = line.find(':');
  if (colon == std::string::npos) {
    throw std::invalid_argument("header line must contain ':'");
  }
  const std::string name = trim(line.substr(0, colon));
  const std::string value = trim(line.substr(colon + 1));
  if (name.empty()) {
    throw std::invalid_argument("header name must not be empty");
  }
  return {name, value};
}

std::vector<std::pair<std::string, std::string>> parse_cookie_string(
    const std::string& cookie_string) {
  std::vector<std::pair<std::string, std::string>> out;
  std::size_t start = 0;
  while (start <= cookie_string.size()) {
    std::size_t semi = cookie_string.find(';', start);
    if (semi == std::string::npos) semi = cookie_string.size();
    const std::string token = trim(cookie_string.substr(start, semi - start));
    if (!token.empty()) {
      const auto eq = token.find('=');
      if (eq == std::string::npos) {
        throw std::invalid_argument("cookie pair must contain '='");
      }
      const std::string name = trim(token.substr(0, eq));
      const std::string value = trim(token.substr(eq + 1));
      if (name.empty()) {
        throw std::invalid_argument("cookie name must not be empty");
      }
      out.emplace_back(name, value);
    }
    if (semi == cookie_string.size()) break;
    start = semi + 1;
  }
  return out;
}

HttpSession::HttpSession() : user_agent_(kDefaultUserAgent) {}

void HttpSession::set_user_agent(const std::string& ua) { user_agent_ = ua; }

void HttpSession::add_header(const std::string& line) {
  headers_.push_back(parse_header_line(line));
}

void HttpSession::set_cookies(const std::string& cookie_string) {
  cookies_ = parse_cookie_string(cookie_string);
}

void HttpSession::set_proxy(const std::string& proxy_url) { proxy_ = proxy_url; }

void HttpSession::set_verify_ssl(bool verify) { verify_ssl_ = verify; }

void HttpSession::set_timeout(int seconds) {
  if (seconds <= 0) {
    throw std::invalid_argument("timeout must be positive");
  }
  timeout_sec_ = seconds;
}

const std::string& HttpSession::user_agent() const { return user_agent_; }

const std::vector<std::pair<std::string, std::string>>& HttpSession::headers() const {
  return headers_;
}

const std::vector<std::pair<std::string, std::string>>& HttpSession::cookies() const {
  return cookies_;
}

const std::string& HttpSession::proxy() const { return proxy_; }

bool HttpSession::verify_ssl() const { return verify_ssl_; }

int HttpSession::timeout_sec() const { return timeout_sec_; }

HttpResponse HttpSession::get(const std::string& url) {
  cpr::Session session;
  configure_cpr_session(session, *this);
  session.SetUrl(cpr::Url{url});
  return from_cpr_response(session.Get());
}

HttpResponse HttpSession::post(const std::string& url, const QueryParams& form) {
  cpr::Session session;
  configure_cpr_session(session, *this);
  session.SetUrl(cpr::Url{url});
  session.SetPayload(to_cpr_payload(form));
  return from_cpr_response(session.Post());
}

HttpSession build_session(const std::vector<std::string>& headers,
                          const std::string& cookie_string,
                          const std::string& proxy) {
  HttpSession session;
  for (const auto& h : headers) {
    session.add_header(h);
  }
  if (!cookie_string.empty()) {
    session.set_cookies(cookie_string);
  }
  if (!proxy.empty()) {
    session.set_proxy(proxy);
  }
  return session;
}

}  // namespace his
