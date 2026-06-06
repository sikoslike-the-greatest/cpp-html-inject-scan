#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

/// \file selector.hpp
/// \brief Интерактивный выбор параметров для тестирования.

namespace his {

/// \brief Разбирает строку выбора в набор индексов.
///
/// Поддерживает: "a" или пустую строку — выбрать все; "n" — ничего;
/// перечисление через запятую с одиночными индексами и диапазонами
/// вида "0,1,5-10". Некорректные и выходящие за границы значения
/// игнорируются. Результат отсортирован и без повторов.
/// \param choice Строка выбора пользователя.
/// \param count Количество доступных элементов.
/// \return Отсортированный список выбранных индексов.
std::vector<std::size_t> parse_selection(const std::string& choice, std::size_t count);

/// \brief Возвращает элементы по списку индексов.
/// \param items Исходный список.
/// \param indices Индексы выбранных элементов.
/// \return Выбранные элементы в порядке индексов.
std::vector<std::string> select_by_indices(const std::vector<std::string>& items,
                                           const std::vector<std::size_t>& indices);

/// \brief Интерактивно выводит список параметров и читает выбор из потока.
/// \param params Доступные имена параметров.
/// \param source_label Метка источника (например, "URL query").
/// \param in Поток ввода.
/// \param out Поток вывода.
/// \return Выбранные имена параметров.
std::vector<std::string> interactive_select(const std::vector<std::string>& params,
                                            const std::string& source_label, std::istream& in,
                                            std::ostream& out);

/// \brief Интерактивный выбор через стандартные потоки ввода-вывода.
/// \param params Доступные имена параметров.
/// \param source_label Метка источника.
/// \return Выбранные имена параметров.
std::vector<std::string> interactive_select(const std::vector<std::string>& params,
                                            const std::string& source_label);

}  // namespace his
