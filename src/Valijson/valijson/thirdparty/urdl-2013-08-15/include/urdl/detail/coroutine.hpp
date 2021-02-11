//
// coroutine.hpp
// ~~~~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef URDL_DETAIL_COROUTINE_HPP
#define URDL_DETAIL_COROUTINE_HPP

#include "urdl/detail/abi_prefix.hpp"

namespace urdl {
namespace detail {

class coroutine
{
protected:
  coroutine() : coro_value_(0) {}
  int coro_value_;
};

#define URDL_CORO_BEGIN \
  switch (this->coroutine::coro_value_) \
  { \
  case 0:

#define URDL_CORO_YIELD_IMPL(s,n) \
  do \
  { \
    this->coroutine::coro_value_ = n; \
    s; \
    return; \
  case n: \
    ; \
  } while (0)

#if defined(_MSC_VER)
# define URDL_CORO_YIELD(s) URDL_CORO_YIELD_IMPL(s, __COUNTER__ + 1)
#else // defined(_MSC_VER)
# define URDL_CORO_YIELD(s) URDL_CORO_YIELD_IMPL(s, __LINE__)
#endif // defined(_MSC_VER)

#define URDL_CORO_END \
  }

} // namespace detail
} // namespace urdl

#include "urdl/detail/abi_suffix.hpp"

#endif // URDL_DETAIL_COROUTINE_HPP
