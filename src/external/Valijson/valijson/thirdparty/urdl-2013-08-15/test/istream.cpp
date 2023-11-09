//
// istream.cpp
// ~~~~~~~~~~~
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
#include "urdl/istream.hpp"

#include "unit_test.hpp"
#include "urdl/http.hpp"
#include "urdl/option_set.hpp"
#include "http_server.hpp"
#include <string>
#include <sstream>

// Ensure all functions compile correctly.
void istream_compile_test()
{
  // Constructors

  urdl::istream istream1;
  urdl::istream istream2("file://foobar");
  urdl::istream istream3(urdl::url("file://foobar"));
  urdl::istream istream4("file://foobar", urdl::option_set());
  urdl::istream istream5(urdl::url("file://foobar"), urdl::option_set());

  // set_option()

  istream1.set_option(0);
  istream1.set_option<char>(0);

  // set_options()

  istream1.set_options(urdl::option_set());

  // get_option()

  const urdl::istream& const_istream1 = istream1;
  want<int>(const_istream1.get_option<int>());
  want<char>(const_istream1.get_option<char>());

  // get_options()

  want<urdl::option_set>(const_istream1.get_options());

  // is_open()

  want<bool>(const_istream1.is_open());

  // open()

  istream1.open("file://foobar");
  istream1.open(urdl::url("file://foobar"));

  // close()

  istream1.close();

  // rdbuf()

  want<urdl::istreambuf*>(const_istream1.rdbuf());

  // error()

  want<boost::system::error_code>(const_istream1.error());

  // read_timeout()

  want<std::size_t>(const_istream1.read_timeout());
  istream1.read_timeout(std::size_t(123));

  // content_type()

  want<std::string>(const_istream1.content_type());

  // content_length()

  want<std::size_t>(const_istream1.content_length());

  // headers()

  want<std::string>(const_istream1.headers());
}

// Test HTTP.
void istream_http_test()
{
  http_server server;
  std::string port = boost::lexical_cast<std::string>(server.port());

  std::string request =
    "GET / HTTP/1.0\r\n"
    "Host: localhost:" + port + "\r\n"
    "Accept: */*\r\n"
    "Connection: close\r\n\r\n";
  std::string response =
    "HTTP/1.0 200 OK\r\n"
    "Content-Length: 13\r\n"
    "Content-Type: text/plain\r\n\r\n";
  std::string content = "Hello, World!";

  server.start(request, 0, response, 0, content);
  urdl::istream istream1("http://localhost:" + port + "/");
  std::string returned_content;
  std::getline(istream1, returned_content);
  bool request_matched = server.stop();

  BOOST_CHECK(request_matched);
  BOOST_CHECK(istream1.content_type() == "text/plain");
  BOOST_CHECK(istream1.content_length() == 13);
  BOOST_CHECK(returned_content == content);
}

// Test HTTP with an error status returned by the server.
void istream_http_not_found_test()
{
  http_server server;
  std::string port = boost::lexical_cast<std::string>(server.port());

  std::string request =
    "GET / HTTP/1.0\r\n"
    "Host: localhost:" + port + "\r\n"
    "Accept: */*\r\n"
    "Connection: close\r\n\r\n";
  std::string response =
    "HTTP/1.0 404 Not Found\r\n"
    "Content-Length: 9\r\n"
    "Content-Type: text/plain\r\n\r\n";
  std::string content = "Not Found";

  server.start(request, 0, response, 0, content);
  urdl::istream istream1("http://localhost:" + port + "/");
  std::string returned_content;
  std::getline(istream1, returned_content);
  bool request_matched = server.stop();

  BOOST_CHECK(request_matched);
  BOOST_CHECK(istream1.error() == urdl::http::errc::not_found);
}

// Test HTTP with an open timeout.
void istream_http_open_timeout_test()
{
  http_server server;
  std::string port = boost::lexical_cast<std::string>(server.port());

  std::string request =
    "GET / HTTP/1.0\r\n"
    "Host: localhost:" + port + "\r\n"
    "Accept: */*\r\n"
    "Connection: close\r\n\r\n";
  std::string response =
    "HTTP/1.0 200 OK\r\n"
    "Content-Length: 13\r\n"
    "Content-Type: text/plain\r\n\r\n";
  std::string content = "Hello, World!";

  server.start(request, 1500, response, 0, content);
  urdl::istream istream1;
  istream1.open_timeout(1000);
  istream1.open("http://localhost:" + port + "/");
  server.stop();

  BOOST_CHECK(istream1.error() == boost::system::errc::timed_out);
}

// Test HTTP with a read timeout.
void istream_http_read_timeout_test()
{
  http_server server;
  std::string port = boost::lexical_cast<std::string>(server.port());

  std::string request =
    "GET / HTTP/1.0\r\n"
    "Host: localhost:" + port + "\r\n"
    "Accept: */*\r\n"
    "Connection: close\r\n\r\n";
  std::string response =
    "HTTP/1.0 200 OK\r\n"
    "Content-Length: 13\r\n"
    "Content-Type: text/plain\r\n\r\n";
  std::string content = "Hello, World!";

  server.start(request, 0, response, 1500, content);
  urdl::istream istream1;
  istream1.open("http://localhost:" + port + "/");
  istream1.read_timeout(1000);
  std::string returned_content;
  std::getline(istream1, returned_content);
  bool request_matched = server.stop();

  BOOST_CHECK(request_matched);
  BOOST_CHECK(istream1.error() == boost::system::errc::timed_out);
}

test_suite* init_unit_test_suite(int, char*[])
{
  test_suite* test = BOOST_TEST_SUITE("istream");
  test->add(BOOST_TEST_CASE(&istream_compile_test));
  test->add(BOOST_TEST_CASE(&istream_http_test));
  test->add(BOOST_TEST_CASE(&istream_http_not_found_test));
  test->add(BOOST_TEST_CASE(&istream_http_open_timeout_test));
  test->add(BOOST_TEST_CASE(&istream_http_read_timeout_test));
  return test;
}
