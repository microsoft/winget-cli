#pragma once

#if __cplusplus >= 201703
// Visual C++ only supports __has_include in versions 14.12 and greater
#  if !defined(_MSC_VER) || _MSC_VER >= 1912
#    if __has_include(<optional>)
#      include <optional>
namespace opt = std;
#    endif
#  endif
#else
#  include <compat/optional.hpp>
namespace opt = std::experimental;
#endif
