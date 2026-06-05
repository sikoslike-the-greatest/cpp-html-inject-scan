#pragma once

#include <string>

/// \file version.hpp
/// \brief Метаданные сборки сканера.

namespace his {

/// \brief Возвращает человекочитаемое имя и версию приложения.
///
/// Значение версии берётся из настроек проекта CMake и встраивается в
/// исполняемый файл на этапе компиляции.
///
/// \return Строка вида "html-inject-scan X.Y.Z".
std::string version_string();

}  // namespace his
