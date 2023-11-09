#pragma once

#include <iostream>
#include <exception>

namespace valijson {
#if defined(_MSC_VER) && _MSC_VER == 1800
#define VALIJSON_NORETURN __declspec(noreturn)
#else
#define VALIJSON_NORETURN [[noreturn]]
#endif

#if VALIJSON_USE_EXCEPTIONS
#include <stdexcept>

VALIJSON_NORETURN inline void throwRuntimeError(const std::string& msg) {
  throw std::runtime_error(msg);
}

VALIJSON_NORETURN inline void throwLogicError(const std::string& msg) {
  throw std::logic_error(msg);
}
#else
VALIJSON_NORETURN inline void throwRuntimeError(const std::string& msg) {
  std::cerr << msg << std::endl;
  abort();
}
VALIJSON_NORETURN inline void throwLogicError(const std::string& msg) {
  std::cerr << msg << std::endl;
  abort();
}

#endif

VALIJSON_NORETURN inline void throwNotSupported() {
    throwRuntimeError("Not supported");                             \
}

} // namespace valijson


