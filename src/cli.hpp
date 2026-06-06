#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "discovery.hpp"

/// \file cli.hpp
/// \brief Разбор аргументов командной строки.

namespace his {

/// \brief Разобранные параметры запуска приложения.
struct Options {
  std::string url_single;     ///< Один целевой URL (-u).
  std::string url_list_path;  ///< Путь к файлу со списком URL (-l).
  std::string payload = "'\"><zxcasd>";  ///< Подставляемый payload (-p).
  std::string marker = "<zxcasd>";       ///< Маркер для поиска отражения (-m).
  std::string wordlist_path;             ///< Путь к словарю параметров (-w).
  std::vector<std::string> extra;        ///< Доп. параметры из --extra.
  bool use_builtin_extra = true;         ///< Использовать встроенный набор (--no-extra выключает).
  std::vector<std::string> headers;      ///< Заголовки "Name: value" (--header).
  std::string cookie;                    ///< Строка cookie (--cookie).
  std::string proxy;                     ///< Прокси (--proxy).
  std::string method = "GET";            ///< HTTP-метод (--method).
  HtmlScanMode mode = HtmlScanMode::Input;  ///< Режим HTML-сканирования (--mode).
  bool no_html_scan = false;             ///< Не загружать страницу для парсинга name= (--no-html-scan).
  bool autoselect = false;               ///< Без интерактива: выбрать все (--auto).
  bool silent = false;                   ///< Тихий режим: только находки (-s).
  std::size_t max_url_len = 2000;        ///< Лимит длины URL для батча (--max-url-len).
  long timeout_ms = 15000;               ///< Таймаут запроса, мс (--timeout).
  std::string output_path;               ///< Файл для сохранения находок (-o).
  bool show_help = false;                ///< Показать справку (-h).
};

/// \brief Разбирает список аргументов (без имени программы) в \ref Options.
/// \param args Аргументы командной строки.
/// \return Заполненная структура параметров.
/// \throws std::invalid_argument при неизвестной опции, отсутствии значения или
///         некорректном числовом/перечислимом значении.
Options parse_args(const std::vector<std::string>& args);

/// \brief Возвращает текст справки по использованию.
/// \return Многострочный текст справки.
std::string usage_text();

}  // namespace his
