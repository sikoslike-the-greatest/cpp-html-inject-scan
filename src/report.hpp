#pragma once

#include <iosfwd>
#include <string>
#include <vector>

#include "scanner.hpp"

/// \file report.hpp
/// \brief Вывод результатов сканирования в консоль и сохранение в файл.

namespace his {

/// \brief Формирует вывод сканера в заданный поток.
///
/// Инкапсулирует ANSI-оформление и режимы вывода. В тихом режиме печатаются
/// только находки и итоговая сводка, без прогресса и заголовков.
class Reporter {
 public:
  /// \brief Создаёт репортер.
  /// \param out Поток вывода.
  /// \param silent Тихий режим (только находки).
  /// \param color Использовать ANSI-цвета.
  Reporter(std::ostream& out, bool silent, bool color);

  /// \brief Печатает сводную информацию о запуске (подавляется в тихом режиме).
  /// \param payload Используемый payload.
  /// \param marker Маркер отражения.
  /// \param method HTTP-метод.
  /// \param url_count Количество целевых URL.
  void run_info(const std::string& payload, const std::string& marker, const std::string& method,
                std::size_t url_count) const;

  /// \brief Печатает заголовок сканирования URL (подавляется в тихом режиме).
  /// \param url Сканируемый URL.
  /// \param param_count Количество выбранных параметров.
  /// \param method HTTP-метод.
  void scan_header(const std::string& url, std::size_t param_count,
                   const std::string& method) const;

  /// \brief Печатает сообщение о пропуске URL (подавляется в тихом режиме).
  /// \param url Пропущенный URL.
  void skipped(const std::string& url) const;

  /// \brief Печатает предупреждение (подавляется в тихом режиме).
  /// \param message Текст предупреждения.
  void warn(const std::string& message) const;

  /// \brief Печатает найденную отражённую инъекцию (печатается всегда).
  /// \param hit Найденная инъекция.
  void hit(const ScanHit& hit) const;

  /// \brief Печатает итоговую сводку (подавляется в тихом режиме).
  /// \param total_hits Общее число находок.
  void summary(std::size_t total_hits) const;

 private:
  std::ostream& out_;
  bool silent_;
  bool color_;

  // Returns the ANSI code when colour is enabled, otherwise an empty string.
  const char* paint(const char* code) const;
};

/// \brief Сохраняет находки в TSV-файл (param \\t status \\t url).
/// \param path Путь к файлу.
/// \param hits Список находок.
/// \throws std::runtime_error если файл не удалось открыть для записи.
void save_hits_tsv(const std::string& path, const std::vector<ScanHit>& hits);

}  // namespace his
