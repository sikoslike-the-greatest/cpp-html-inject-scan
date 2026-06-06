#pragma once

#include <string>
#include <vector>

/// \file discovery.hpp
/// \brief Обнаружение имён параметров из URL, HTML, словаря и встроенного набора.

namespace his {

/// \brief Режим извлечения параметров из HTML.
enum class HtmlScanMode {
  Input,  ///< Только атрибуты name= у тегов <input>.
  All,    ///< Атрибуты name= у любых тегов.
};

/// \brief Извлекает имена query-параметров из URL.
///
/// Тонкая обёртка над \ref extract_query_params для единообразия источников.
/// \param url Исходный URL.
/// \return Уникальные имена параметров в порядке появления.
std::vector<std::string> params_from_url(const std::string& url);

/// \brief Извлекает имена параметров из HTML по атрибутам name=.
///
/// В режиме \ref HtmlScanMode::Input учитываются только теги <input>,
/// в режиме \ref HtmlScanMode::All — любые теги с атрибутом name=.
/// \param html Текст HTML-страницы.
/// \param mode Режим сканирования.
/// \return Уникальные имена параметров в порядке появления.
std::vector<std::string> params_from_html(const std::string& html, HtmlScanMode mode);

/// \brief Загружает словарь имён параметров из текстового файла.
///
/// Каждая непустая строка (после обрезки пробелов) — отдельный параметр.
/// \param path Путь к файлу словаря.
/// \return Список имён параметров.
/// \throws std::runtime_error если файл не удалось открыть.
std::vector<std::string> load_wordlist(const std::string& path);

/// \brief Возвращает встроенный набор часто встречающихся имён параметров.
/// \return Ссылка на статический список имён.
const std::vector<std::string>& default_extra_params();

/// \brief Удаляет дубликаты, сохраняя порядок первого появления.
/// \param items Исходный список.
/// \return Список без повторов.
std::vector<std::string> dedup_preserve_order(const std::vector<std::string>& items);

}  // namespace his
