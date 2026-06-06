#pragma once

#include <string>
#include <utility>
#include <vector>

/// \file url_utils.hpp
/// \brief Разбор URL, работа с query string и сборка тестовых URL с payload.

namespace his {

/// \brief Компоненты разобранного URL.
///
/// Хранит части URL так, чтобы их можно было модифицировать (например,
/// заменить query) и собрать обратно функцией \ref build_url.
struct ParsedUrl {
  std::string scheme;     ///< Схема без "://", например "https".
  std::string authority;  ///< Хост с опциональным портом и userinfo.
  std::string path;       ///< Путь, начинающийся с '/', либо пустая строка.
  std::string query;      ///< Query string без ведущего '?'.
  std::string fragment;   ///< Фрагмент без ведущего '#'.
};

/// \brief Упорядоченный список пар "ключ-значение" query string.
///
/// Используется вместо ассоциативного контейнера, чтобы сохранять порядок
/// параметров и допускать повторяющиеся ключи (как в исходном URL).
using QueryParams = std::vector<std::pair<std::string, std::string>>;

/// \brief Разбирает URL на составные части.
/// \param url Исходный URL (ожидается со схемой, например "https://...").
/// \return Структура \ref ParsedUrl с заполненными компонентами.
ParsedUrl parse_url(const std::string& url);

/// \brief Процентное (percent) кодирование строки для query-компонента.
///
/// Незарезервированные символы (A-Z a-z 0-9 - _ . ~) остаются как есть,
/// остальные кодируются в виде %XX в верхнем регистре.
/// \param value Исходная строка.
/// \return Закодированная строка.
std::string url_encode(const std::string& value);

/// \brief Декодирует процентное кодирование, '+' трактуется как пробел.
/// \param value Закодированная строка.
/// \return Декодированная строка.
std::string url_decode(const std::string& value);

/// \brief Разбирает query string в упорядоченный список пар.
/// \param query Query string без ведущего '?'.
/// \return Список пар ключ-значение (значения и ключи декодированы).
QueryParams parse_query(const std::string& query);

/// \brief Собирает query string из списка пар с percent-кодированием.
/// \param params Список пар ключ-значение.
/// \return Готовая query string без ведущего '?'.
std::string build_query(const QueryParams& params);

/// \brief Собирает URL обратно из компонентов.
/// \param parts Компоненты URL.
/// \return Строка URL.
std::string build_url(const ParsedUrl& parts);

/// \brief Извлекает имена query-параметров из URL.
///
/// Порядок сохраняется, повторяющиеся имена возвращаются один раз.
/// \param url Исходный URL.
/// \return Список уникальных имён параметров.
std::vector<std::string> extract_query_params(const std::string& url);

/// \brief Строит URL, подставляя одно значение в заданные параметры.
///
/// Существующие параметры с такими именами получают новое значение,
/// отсутствующие добавляются в конец query. Прочие параметры исходного
/// URL сохраняются без изменений.
/// \param url Базовый URL.
/// \param params Имена параметров, в которые подставляется значение.
/// \param value Значение (payload), общее для всех указанных параметров.
/// \return Готовый URL с подставленным значением.
std::string build_url_with_params(const std::string& url, const std::vector<std::string>& params,
                                  const std::string& value);

}  // namespace his
