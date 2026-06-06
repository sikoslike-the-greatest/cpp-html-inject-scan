#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "http_client.hpp"

/// \file scanner.hpp
/// \brief Подстановка payload, проверка отражения marker, батчинг и bisect.

namespace his {

/// \brief Функция отправки запроса: по URL возвращает ответ сервера.
///
/// Абстрагирует сетевой слой, чтобы сканер можно было тестировать без сети.
using ResponseFn = std::function<HttpResponse(const std::string&)>;

/// \brief Найденная отражённая инъекция (одиночный параметр).
struct ScanHit {
  std::string param;     ///< Имя уязвимого параметра.
  std::string url;       ///< URL с payload, подставленным в этот параметр.
  long status = 0;       ///< HTTP статус ответа.
  std::size_t length{};  ///< Длина тела ответа.
};

/// \brief Результат одной пробы (запроса для набора параметров).
struct ProbeResult {
  std::string url;         ///< Отправленный URL.
  long status = 0;         ///< HTTP статус ответа.
  bool reflected = false;  ///< true, если marker найден в теле.
  std::size_t length{};    ///< Длина тела ответа.
};

/// \brief Проверяет, содержится ли marker в теле ответа.
/// \param body Тело ответа.
/// \param marker Искомая подстрока.
/// \return true, если marker присутствует.
bool is_reflected(const std::string& body, const std::string& marker);

/// \brief Выполняет пробу: подставляет payload в параметры и шлёт запрос.
/// \param base_url Базовый URL.
/// \param params Параметры, в которые подставляется payload.
/// \param payload Подставляемое значение.
/// \param marker Подстрока для поиска отражения.
/// \param send Функция отправки запроса.
/// \return Результат пробы с признаком отражения.
ProbeResult probe(const std::string& base_url, const std::vector<std::string>& params,
                  const std::string& payload, const std::string& marker, const ResponseFn& send);

/// \brief Определяет, сколько параметров с начала start укладывается в лимит длины URL.
///
/// Возвращает минимум 1, даже если один параметр уже превышает лимит,
/// чтобы сканирование всегда продвигалось.
/// \param base_url Базовый URL.
/// \param params Полный список параметров.
/// \param start Индекс начала батча.
/// \param payload Подставляемое значение (учитывается его длина после кодирования).
/// \param max_url_len Максимально допустимая длина URL.
/// \return Размер батча (число параметров).
std::size_t pack_batch(const std::string& base_url, const std::vector<std::string>& params,
                       std::size_t start, const std::string& payload, std::size_t max_url_len);

/// \brief Бинарным делением находит отражающие параметры внутри батча.
///
/// Предусловие: батч как целое уже отражает marker. Делит батч пополам и
/// рекурсивно сужает поиск, выполняя логарифмическое число запросов.
/// \param base_url Базовый URL.
/// \param batch Параметры батча (известно, что батч отражает).
/// \param payload Подставляемое значение.
/// \param marker Подстрока для поиска отражения.
/// \param send Функция отправки запроса.
/// \return Список отражающих параметров.
std::vector<std::string> find_reflected(const std::string& base_url,
                                        const std::vector<std::string>& batch,
                                        const std::string& payload, const std::string& marker,
                                        const ResponseFn& send);

/// \brief Сканирует URL: батчит параметры под лимит длины и выделяет отражающие.
/// \param base_url Базовый URL.
/// \param params Параметры для тестирования.
/// \param payload Подставляемое значение.
/// \param marker Подстрока для поиска отражения.
/// \param max_url_len Максимальная длина URL для батча.
/// \param send Функция отправки запроса.
/// \return Список найденных отражённых инъекций.
std::vector<ScanHit> scan_url(const std::string& base_url, const std::vector<std::string>& params,
                              const std::string& payload, const std::string& marker,
                              std::size_t max_url_len, const ResponseFn& send);

}  // namespace his
