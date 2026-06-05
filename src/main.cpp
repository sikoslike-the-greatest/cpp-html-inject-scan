#include <iostream>

#include "version.hpp"

/// \file main.cpp
/// \brief Точка входа приложения. На этапе скелета только выводит версию.

int main() {
  try {
    std::cout << his::version_string() << "\n";
    std::cout << "Skeleton build. Functionality is added module by module.\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "fatal: " << e.what() << "\n";
    return 1;
  }
}
