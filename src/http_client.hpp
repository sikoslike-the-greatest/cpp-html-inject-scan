#pragma once

#include <string>
#include <utility>
#include <vector>

#include "url_utils.hpp"

/// \file http_client.hpp
/// \brief HTTP-сессия с поддержкой заголовков, cookie и прокси поверх cpr.

namespace his {

/// \brief Результат HTTP-запроса.
struct HttpResponse {
  long status = 0;       ///< HTTP статус-код ответа (например, 200).
  std::string text;      ///< Тело ответа целиком.
  std::size_t length{};  ///< Длина тела ответа в байтах.
};

/// \brief Разбирает строку заголовка вида "Name: value".
///
/// Делит по первому двоеточию, обрезает пробелы вокруг имени и значения.
/// \param line Строка заголовка.
/// \return Пара (имя, значение).
/// \throws std::invalid_argument если двоеточие отсутствует.
std::pair<std::string, std::string> parse_header_line(const std::string& line);

/// \brief Разбирает строку cookie вида "k=v; k2=v2".
/// \param cookies Строка cookie, пары разделены ';'.
/// \return Список пар (имя, значение); пустые сегменты пропускаются.
std::vector<std::pair<std::string, std::string>> parse_cookie_string(const std::string& cookies);

/// \brief HTTP-сессия: хранит общие для запросов настройки и выполняет GET/POST.
///
/// Повторяет поведение build_session из исходного Python-инструмента:
/// задаёт User-Agent, пользовательские заголовки, cookie, прокси и
/// отключение проверки TLS-сертификата (для тестирования через прокси).
class Session {
 public:
  /// \brief Создаёт сессию с браузерным User-Agent и отключённой проверкой TLS.
  Session();

  /// \brief Устанавливает строку User-Agent.
  /// \param ua Значение заголовка User-Agent.
  void set_user_agent(const std::string& ua);

  /// \brief Добавляет/перезаписывает заголовок.
  /// \param name Имя заголовка.
  /// \param value Значение заголовка.
  void add_header(const std::string& name, const std::string& value);

  /// \brief Добавляет заголовок из строки "Name: value".
  /// \param line Строка заголовка.
  void add_header_line(const std::string& line);

  /// \brief Задаёт cookie из строки "k=v; k2=v2".
  /// \param cookie_str Строка cookie.
  void set_cookies(const std::string& cookie_str);

  /// \brief Задаёт HTTP/HTTPS прокси.
  /// \param proxy URL прокси, например "http://127.0.0.1:8080".
  void set_proxy(const std::string& proxy);

  /// \brief Задаёт таймаут запроса в миллисекундах.
  /// \param ms Таймаут, мс.
  void set_timeout_ms(long ms);

  /// \brief Включает/выключает проверку TLS-сертификата.
  /// \param verify true — проверять сертификат.
  void set_verify_ssl(bool verify);

  /// \brief Выполняет GET-запрос с текущими настройками сессии.
  /// \param url Целевой URL.
  /// \return Ответ сервера.
  /// \throws std::runtime_error при транспортной ошибке (DNS, соединение, таймаут).
  HttpResponse get(const std::string& url) const;

  /// \brief Выполняет POST-запрос с телом из переданных пар (form-urlencoded).
  /// \param url Целевой URL (без query, либо query игнорируется телом).
  /// \param form Пары ключ-значение тела запроса.
  /// \return Ответ сервера.
  /// \throws std::runtime_error при транспортной ошибке.
  HttpResponse post(const std::string& url, const QueryParams& form) const;

 private:
  std::vector<std::pair<std::string, std::string>> headers_;
  std::vector<std::pair<std::string, std::string>> cookies_;
  std::string user_agent_;
  std::string proxy_;
  long timeout_ms_ = 15000;
  bool verify_ssl_ = false;
};

}  // namespace his
