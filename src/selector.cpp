#include "selector.hpp"

#include <algorithm>
#include <charconv>
#include <iostream>
#include <optional>
#include <set>

namespace his {

namespace {

std::string trim(const std::string& s) {
  const auto begin = s.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) return std::string();
  const auto end = s.find_last_not_of(" \t\r\n");
  return s.substr(begin, end - begin + 1);
}

std::string to_lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

// Parses a non-negative integer; returns nullopt for empty/invalid input.
std::optional<long> parse_int(const std::string& token) {
  const std::string t = trim(token);
  if (t.empty()) return std::nullopt;
  long value = 0;
  const char* first = t.data();
  const char* last = t.data() + t.size();
  const auto result = std::from_chars(first, last, value);
  if (result.ec != std::errc() || result.ptr != last) return std::nullopt;
  return value;
}

void add_if_in_range(std::set<std::size_t>& sel, long index, std::size_t count) {
  if (index >= 0 && static_cast<std::size_t>(index) < count) {
    sel.insert(static_cast<std::size_t>(index));
  }
}

void parse_part(const std::string& part, std::size_t count, std::set<std::size_t>& sel) {
  const auto dash = part.find('-');
  if (dash != std::string::npos) {
    const auto lo = parse_int(part.substr(0, dash));
    const auto hi = parse_int(part.substr(dash + 1));
    if (lo && hi) {
      for (long x = *lo; x <= *hi; ++x) add_if_in_range(sel, x, count);
    }
  } else if (const auto idx = parse_int(part)) {
    add_if_in_range(sel, *idx, count);
  }
}

}  // namespace

std::vector<std::size_t> parse_selection(const std::string& choice, std::size_t count) {
  const std::string c = to_lower(trim(choice));

  if (c.empty() || c == "a") {
    std::vector<std::size_t> all(count);
    for (std::size_t i = 0; i < count; ++i) all[i] = i;
    return all;
  }
  if (c == "n") return {};

  std::set<std::size_t> sel;
  std::size_t start = 0;
  while (start <= c.size()) {
    std::size_t comma = c.find(',', start);
    if (comma == std::string::npos) comma = c.size();
    parse_part(c.substr(start, comma - start), count, sel);
    if (comma == c.size()) break;
    start = comma + 1;
  }
  return std::vector<std::size_t>(sel.begin(), sel.end());
}

std::vector<std::string> select_by_indices(const std::vector<std::string>& items,
                                           const std::vector<std::size_t>& indices) {
  std::vector<std::string> out;
  out.reserve(indices.size());
  for (const auto i : indices) {
    if (i < items.size()) out.push_back(items[i]);
  }
  return out;
}

std::vector<std::string> interactive_select(const std::vector<std::string>& params,
                                             const std::string& source_label, std::istream& in,
                                             std::ostream& out) {
  if (params.empty()) return {};

  out << "\n[" << source_label << "] Found " << params.size() << " params:\n";
  for (std::size_t i = 0; i < params.size(); ++i) {
    out << "  " << i << "  " << params[i] << "\n";
  }
  out << "  a = all, n = none, e.g. 0,1,5-10 = pick by index\n  > ";

  std::string choice;
  std::getline(in, choice);
  return select_by_indices(params, parse_selection(choice, params.size()));
}

std::vector<std::string> interactive_select(const std::vector<std::string>& params,
                                             const std::string& source_label) {
  return interactive_select(params, source_label, std::cin, std::cout);
}

}  // namespace his
