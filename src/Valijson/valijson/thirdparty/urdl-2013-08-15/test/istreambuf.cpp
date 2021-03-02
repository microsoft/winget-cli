//
// istreambuf.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Disable autolinking for unit tests.
#if !defined(BOOST_ALL_NO_LIB)
#define BOOST_ALL_NO_LIB 1
#endif // !defined(BOOST_ALL_NO_LIB)

// Test that header file is self-contained.
#include "urdl/istreambuf.hpp"

#include "unit_test.hpp"
#include "urdl/option_set.hpp"

// Ensure all functions compile correctly.
void istreambuf_compile_test()
{
  // Constructors

  urdl::istreambuf istreambuf1;

  // set_option()

  istreambuf1.set_option(0);
  istreambuf1.set_option<char>(0);

  // set_options()

  istreambuf1.set_options(urdl::option_set());

  // get_option()

  const urdl::istreambuf& const_istreambuf1 = istreambuf1;
  want<int>(const_istreambuf1.get_option<int>());
  want<char>(const_istreambuf1.get_option<char>());

  // get_options()

  want<urdl::option_set>(const_istreambuf1.get_options());

  // is_open()

  want<bool>(const_istreambuf1.is_open());

  // open()

  want<urdl::istreambuf*>(istreambuf1.open("file://foobar"));
  want<urdl::istreambuf*>(istreambuf1.open(urdl::url("file://foobar")));

  // close()

  want<urdl::istreambuf*>(istreambuf1.close());

  // puberror()

  want<boost::system::error_code>(const_istreambuf1.puberror());

  // read_timeout()

  want<std::size_t>(const_istreambuf1.read_timeout());
  istreambuf1.read_timeout(std::size_t(123));

  // content_type()

  want<std::string>(const_istreambuf1.content_type());

  // content_length()

  want<std::size_t>(const_istreambuf1.content_length());

  // headers()

  want<std::string>(const_istreambuf1.headers());
}

test_suite* init_unit_test_suite(int, char*[])
{
  test_suite* test = BOOST_TEST_SUITE("istreambuf");
  test->add(BOOST_TEST_CASE(&istreambuf_compile_test));
  return test;
}
