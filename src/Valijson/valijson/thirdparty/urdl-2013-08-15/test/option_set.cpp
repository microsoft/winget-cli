//
// option_set.cpp
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
#include "urdl/option_set.hpp"

#include "unit_test.hpp"

// Ensure all functions compile correctly.
void option_set_compile_test()
{
  // Constructors

  urdl::option_set set1;
  urdl::option_set set2(set1);

  // operator=

  set1 = set2;

  // set_option()

  set1.set_option(0);
  set1.set_option<char>(0);

  // set_options()

  const urdl::option_set& const_set1 = set1;
  set2.set_options(const_set1);

  // get_option()

  want<int>(const_set1.get_option<int>());
  want<char>(const_set1.get_option<char>());

  // clear_option()

  set1.clear_option<int>();
  set1.clear_option<char>();
}

// Test option_set runtime behaviour.
void option_set_runtime_test()
{
  urdl::option_set set1;
  set1.set_option(std::string("foobar"));
  set1.set_option(int(123));

  urdl::option_set set2(set1);
  BOOST_CHECK(set2.get_option<std::string>() == set1.get_option<std::string>());
  BOOST_CHECK(set2.get_option<int>() == set1.get_option<int>());

  urdl::option_set set3;
  set3 = set2;
  BOOST_CHECK(set3.get_option<std::string>() == set1.get_option<std::string>());
  BOOST_CHECK(set3.get_option<int>() == set1.get_option<int>());

  urdl::option_set set4;
  set4.set_option(char('A'));
  set4.set_options(set3);
  BOOST_CHECK(set4.get_option<char>() == 'A');
  BOOST_CHECK(set4.get_option<std::string>() == set1.get_option<std::string>());
  BOOST_CHECK(set4.get_option<int>() == set1.get_option<int>());

  set4.clear_option<std::string>();
  BOOST_CHECK(set4.get_option<char>() == 'A');
  BOOST_CHECK(set4.get_option<std::string>() == "");
  BOOST_CHECK(set4.get_option<int>() == set1.get_option<int>());

  set4.set_option(int(321));
  BOOST_CHECK(set4.get_option<char>() == 'A');
  BOOST_CHECK(set4.get_option<std::string>() == "");
  BOOST_CHECK(set4.get_option<int>() == 321);

  set4.set_options(set1);
  BOOST_CHECK(set4.get_option<char>() == 'A');
  BOOST_CHECK(set4.get_option<std::string>() == set1.get_option<std::string>());
  BOOST_CHECK(set4.get_option<int>() == set1.get_option<int>());
}

test_suite* init_unit_test_suite(int, char*[])
{
  test_suite* test = BOOST_TEST_SUITE("option_set");
  test->add(BOOST_TEST_CASE(&option_set_compile_test));
  test->add(BOOST_TEST_CASE(&option_set_runtime_test));
  return test;
}
