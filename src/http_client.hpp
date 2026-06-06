#pragma once

#include <string>
#include <utility>
#include <vector>

#include "url_utils.hpp"

/// \file http_client.hpp
/// \brief HTTP-сессия с поддержкой заголовков, cookie, прокси, GET и POST.

namespace his {

/// \brief User-Agent по умолчанию (как в Python-версии сканера).
constexpr const char* kDefaultUserAgent =
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36";

/// \brief Таймаут запроса по умолчанию, секунды.
constexpr int kDefaultTimeoutSec = 15;

/// \brief Результат HTTP-запроса.
struct HttpResponse {
  int status_code = 0;  ///< HTTP-код ответа (0 при сетевой ошибке до ответа).
  std::string body;     ///< Тело ответа.
};

/// \brief Разбирает строку заголовка формата "Name: value".
/// \param line Строка заголовка.
/// \return Пара (имя, значение) с обрезанными пробелами.
/// \throws std::invalid_argument если ':' отсутствует или имя пустое.
std::pair<std::string, std::string> parse_header_line(const std::string& line);

/// \brief Разбирает cookie-строку формата "name=val; name2=val2".
/// \param cookie_string Строка cookie.
/// \return Список пар имя-значение.
/// \throws std::invalid_argument если встречена пара без '=' или с пустым именем.
std::vector<std::pair<std::string, std::string>> parse_cookie_string(
    const std::string& cookie_string);

/// \brief HTTP-сессия: переиспользуемые настройки и выполнение GET/POST.
///
/// По умолчанию: User-Agent сканера, SSL-проверка отключена, редиректы
/// разрешены, таймаут 15 с (как в Python-версии).
class HttpSession {
 public:
  /// \brief Создаёт сессию с настройками по умолчанию.
  HttpSession();

  /// \brief Устанавливает User-Agent.
  /// \param ua Строка User-Agent.
  void set_user_agent(const std::string& ua);

  /// \brief Добавляет заголовок из строки "Name: value".
  /// \param line Строка заголовка.
  void add_header(const std::string& line);

  /// \brief Устанавливает cookie из строки "a=1; b=2".
  /// \param cookie_string Строка cookie.
  void set_cookies(const std::string& cookie_string);

  /// \brief Устанавливает HTTP-прокси для http и https.
  /// \param proxy_url URL прокси, например "http://127.0.0.1:8080".
  void set_proxy(const std::string& proxy_url);

  /// \brief Включает или отключает проверку SSL-сертификата.
  /// \param verify true — проверять сертификат.
  void set_verify_ssl(bool verify);

  /// \brief Устанавливает таймаут запроса в секундах.
  /// \param seconds Таймаут (> 0).
  void set_timeout(int seconds);

  /// \brief Выполняет GET-запрос.
  /// \param url Целевой URL.
  /// \return Ответ сервера.
  /// \throws std::runtime_error при сетевой ошибке cpr/curl.
  HttpResponse get(const std::string& url);

  /// \brief Выполняет POST с form-urlencoded телом.
  /// \param url Целевой URL (обычно без query string).
  /// \param form Пары ключ-значение для тела запроса.
  /// \return Ответ сервера.
  /// \throws std::runtime_error при сетевой ошибке cpr/curl.
  HttpResponse post(const std::string& url, const QueryParams& form);

  /// \brief Возвращает текущий User-Agent сессии.
  /// \return Строка User-Agent.
  const std::string& user_agent() const;

  /// \brief Возвращает настроенные дополнительные заголовки.
  /// \return Список пар имя-значение.
  const std::vector<std::pair<std::string, std::string>>& headers() const;

  /// \brief Возвращает настроенные cookie.
  /// \return Список пар имя-значение.
  const std::vector<std::pair<std::string, std::string>>& cookies() const;

  /// \brief Возвращает URL прокси (пустая строка, если не задан).
  /// \return URL прокси.
  const std::string& proxy() const;

  /// \brief Возвращает флаг проверки SSL.
  /// \return true, если проверка включена.
  bool verify_ssl() const;

  /// \brief Возвращает таймаут запроса в секундах.
  /// \return Таймаут в секундах.
  int timeout_sec() const;

 private:
  std::string user_agent_;
  std::vector<std::pair<std::string, std::string>> headers_;
  std::vector<std::pair<std::string, std::string>> cookies_;
  std::string proxy_;
  bool verify_ssl_ = false;
  int timeout_sec_ = kDefaultTimeoutSec;
};

/// \brief Создаёт сессию из списка заголовков, cookie и прокси (как build_session в Python).
/// \param headers Список строк "Name: value" (может быть пустым).
/// \param cookie_string Строка cookie или пустая.
/// \param proxy URL прокси или пустая строка.
/// \return Настроенная \ref HttpSession.
HttpSession build_session(const std::vector<std::string>& headers,
                          const std::string& cookie_string,
                          const std::string& proxy);

}  // namespace his
