#if defined(URDL_HEADER_ONLY)
# error Do not compile Urdl library source with URDL_HEADER_ONLY defined
#endif

#define URDL_SOURCE

#include "urdl/istreambuf.hpp"
#include "urdl/impl/istreambuf.ipp"

#include "urdl/http.hpp"
#include "urdl/impl/http.ipp"

#include "urdl/option_set.hpp"
#include "urdl/impl/option_set.ipp"

#include "urdl/url.hpp"
#include "urdl/impl/url.ipp"
