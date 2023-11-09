//
// unit_test.hpp
// ~~~~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef UNIT_TEST_HPP
#define UNIT_TEST_HPP

#include <boost/config.hpp>

#if defined(BOOST_MSVC)
# pragma warning (push)
# pragma warning (disable:4244)
# pragma warning (disable:4535)
# pragma warning (disable:4702)
# pragma warning (disable:4996)
#endif // defined(BOOST_MSVC)

#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test_framework.hpp>
using boost::unit_test::test_suite;

#if defined(BOOST_MSVC)
# pragma warning (pop)
# pragma warning (disable:4996) // Leave this disabled for the unit tests.
#endif // defined(BOOST_MSVC)

// Helper function to check the return type of a function.
template <typename T>
void want(const T&)
{
}

#endif // UNIT_TEST_HPP
