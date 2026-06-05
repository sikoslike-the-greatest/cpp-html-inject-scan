#include "version.hpp"

#ifndef HIS_VERSION
#define HIS_VERSION "0.0.0"
#endif

namespace his {

std::string version_string() { return std::string("html-inject-scan ") + HIS_VERSION; }

}  // namespace his
