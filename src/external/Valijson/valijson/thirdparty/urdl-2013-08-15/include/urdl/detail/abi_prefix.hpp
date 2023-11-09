//
// abi_prefix.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// No include guard.

#include <boost/config/abi_prefix.hpp>

// Disable some pesky MSVC warnings.
#if defined(_MSC_VER)
# pragma warning (push)
# pragma warning (disable:4127)
# pragma warning (disable:4251)
# pragma warning (disable:4355)
# pragma warning (disable:4512)
# pragma warning (disable:4996)
#endif // defined(_MSC_VER)

// Force external visibility of all types.
#if defined(__GNUC__)
# if (__GNUC__ == 4 && __GNUC_MINOR__ >= 1) || (__GNUC__ > 4)
#  pragma GCC visibility push (default)
# endif // (__GNUC__ == 4 && __GNUC_MINOR__ >= 1) || (__GNUC__ > 4)
#endif // defined(__GNUC__)

