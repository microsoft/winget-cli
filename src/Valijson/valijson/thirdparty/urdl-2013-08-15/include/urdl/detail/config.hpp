//
// config.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// No include guard.

#include <boost/config.hpp>
#include <boost/version.hpp>

#if defined(URDL_HEADER_ONLY)
# define URDL_DECL inline
#else // defined(URDL_HEADER_ONLY)
# if defined(BOOST_HAS_DECLSPEC)
// We need to import/export our code only if the user has specifically asked
// for it by defining either BOOST_ALL_DYN_LINK if they want all boost
// libraries to be dynamically linked (and if boost is dynamically linked, urdl
// must be dynamically linked too), or URDL_DYN_LINK if they want just urdl to
// be dynamically liked.
#  if defined(BOOST_ALL_DYN_LINK) || defined(URDL_DYN_LINK)
#   if !defined(URDL_DYN_LINK)
#    define URDL_DYN_LINK
#   endif // !defined(URDL_DYN_LINK)
// Export if this is our own source, otherwise import.
#   if defined(URDL_SOURCE)
#    define URDL_DECL __declspec(dllexport)
#   else // defined(URDL_SOURCE)
#    define URDL_DECL __declspec(dllimport)
#   endif // defined(URDL_SOURCE)
#  endif // defined(BOOST_ALL_DYN_LINK) || defined(URDL_DYN_LINK)
# endif // defined(BOOST_HAS_DECLSPEC)
#endif // defined(URDL_HEADER_ONLY)

// If URDL_DECL isn't defined yet define it now.
#if !defined(URDL_DECL)
# define URDL_DECL
#endif // !defined(URDL_DECL)

#if !defined(URDL_ERROR_CATEGORY_NOEXCEPT)
# if (BOOST_VERSION >= 105300)
#  define URDL_ERROR_CATEGORY_NOEXCEPT BOOST_NOEXCEPT
# else // (BOOOST_VERSION >= 105300)
#  define URDL_ERROR_CATEGORY_NOEXCEPT
# endif // (BOOOST_VERSION >= 105300)
#endif // !defined(URDL_ERROR_CATEGORY_NOEXCEPT)

#if (BOOST_VERSION >= 105400)
# define URDL_INITFN_RESULT_TYPE(h, sig) BOOST_ASIO_INITFN_RESULT_TYPE(h, sig)
#else // (BOOST_VERSION >= 105400)
# define URDL_INITFN_RESULT_TYPE(h, sig) void
#endif // (BOOST_VERSION >= 105400)

// Enable library autolinking for MSVC.

#if !defined(BOOST_ALL_NO_LIB) && !defined(URDL_NO_LIB) \
  && !defined(URDL_SOURCE) && !defined(URDL_HEADER_ONLY) \
  && defined(_MSC_VER)

# if !defined(_MT)
#  error "You must use the multithreaded runtime."
# endif

# if (defined(_DLL) || defined(_RTLDLL)) && defined(URDL_DYN_LINK)
#  define URDL_LIB_PREFIX
# elif defined(URDL_DYN_LINK)
#  error "Mixing a dll library with a static runtime is unsupported."
# else
#  define URDL_LIB_PREFIX "lib"
# endif

# if defined(_DEBUG)
#  if defined(_DLL)
#   define URDL_LIB_SUFFIX "-gd"
#  else
#   define URDL_LIB_SUFFIX "-sgd"
#  endif
# else
#  if defined(_DLL)
#   define URDL_LIB_SUFFIX
#  else
#   define URDL_LIB_SUFFIX "-s"
#  endif
# endif

# pragma comment(lib, URDL_LIB_PREFIX "urdl" URDL_LIB_SUFFIX ".lib")

#endif // !defined(BOOST_ALL_NO_LIB) && !defined(URDL_NO_LIB)
       //  && !defined(URDL_SOURCE) && !defined(URDL_HEADER_ONLY)
       //  && defined(_MSC_VER)
