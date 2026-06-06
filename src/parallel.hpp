#pragma once

#include <algorithm>
#include <cstddef>
#include <future>
#include <type_traits>
#include <vector>

/// \file parallel.hpp
/// \brief Параллельное применение функции к элементам с сохранением порядка.

namespace his {

/// \brief Применяет функцию к каждому элементу, выполняя до max_threads задач одновременно.
///
/// Элементы обрабатываются волнами по max_threads штук через std::async.
/// Порядок результатов соответствует порядку входных элементов. Исключения
/// из задач пробрасываются при получении результата.
/// \tparam T Тип элемента.
/// \tparam Fn Тип функции, принимающей const T& и возвращающей результат.
/// \param items Входные элементы.
/// \param fn Применяемая функция.
/// \param max_threads Максимум одновременных задач (значение < 1 трактуется как 1).
/// \return Результаты в порядке входных элементов.
template <typename T, typename Fn>
auto parallel_map(const std::vector<T>& items, Fn fn, std::size_t max_threads)
    -> std::vector<std::invoke_result_t<Fn, const T&>> {
  using Result = std::invoke_result_t<Fn, const T&>;
  std::vector<Result> results(items.size());
  if (max_threads < 1) max_threads = 1;

  std::size_t i = 0;
  while (i < items.size()) {
    const std::size_t chunk = std::min(max_threads, items.size() - i);
    std::vector<std::future<Result>> futures;
    futures.reserve(chunk);
    for (std::size_t j = 0; j < chunk; ++j) {
      const std::size_t index = i + j;
      futures.push_back(
          std::async(std::launch::async, [&fn, &items, index]() { return fn(items[index]); }));
    }
    for (std::size_t j = 0; j < chunk; ++j) {
      results[i + j] = futures[j].get();
    }
    i += chunk;
  }
  return results;
}

}  // namespace his
