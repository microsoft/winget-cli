//
// read_stream.cpp
// ~~~~~~~~~~~~~~~
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
#include "urdl/read_stream.hpp"

#include "unit_test.hpp"
#include "urdl/option_set.hpp"
#include "http_server.hpp"
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read.hpp>

void open_handler(const boost::system::error_code&) {}
void read_handler(const boost::system::error_code&, std::size_t) {}

// Ensure all functions compile correctly.
void read_stream_compile_test()
{
  try
  {
    boost::asio::io_service io_service;
    boost::system::error_code ec;
    char buffer[1024];

    // Constructors

    urdl::read_stream stream1(io_service);

    // get_io_service()

    want<boost::asio::io_service>(stream1.get_io_service());

    // set_option()

    stream1.set_option(0);
    stream1.set_option<char>(0);

    // set_options()

    stream1.set_options(urdl::option_set());

    // get_option()

    const urdl::read_stream& const_stream1 = stream1;
    want<int>(const_stream1.get_option<int>());
    want<char>(const_stream1.get_option<char>());

    // get_options()

    want<urdl::option_set>(const_stream1.get_options());

    // is_open()

    want<bool>(const_stream1.is_open());

    // open()

    stream1.open("file://xyz");
    stream1.open(urdl::url("file://xyz"));
    want<boost::system::error_code>(stream1.open("file://xyz", ec));
    want<boost::system::error_code>(stream1.open(urdl::url("file://xyz"), ec));

    // async_open()

    stream1.async_open("file://xyz", open_handler);
    stream1.async_open(urdl::url("file://xyz"), open_handler);

    // close()

    stream1.close();
    want<boost::system::error_code>(stream1.close(ec));

    // content_type()

    want<std::string>(const_stream1.content_type());

    // content_length()

    want<std::size_t>(const_stream1.content_length());

    // headers()

    want<std::string>(const_stream1.headers());

    // read_some()

    want<std::size_t>(stream1.read_some(boost::asio::buffer(buffer)));
    want<std::size_t>(stream1.read_some(boost::asio::buffer(buffer), ec));

    // async_read_some()

    stream1.async_read_some(boost::asio::buffer(buffer), read_handler);
  }
  catch (std::exception&)
  {
  }
}

// Test synchronous HTTP.
void read_stream_synchronous_http_test()
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

  boost::asio::io_service io_service;
  urdl::read_stream stream1(io_service);

  stream1.open("http://localhost:" + port + "/");

  std::string returned_content(stream1.content_length(), 0);
  boost::asio::read(stream1, boost::asio::buffer(
        &returned_content[0], returned_content.size()));

  bool request_matched = server.stop();

  BOOST_CHECK(request_matched);
  BOOST_CHECK(stream1.content_type() == "text/plain");
  BOOST_CHECK(stream1.content_length() == 13);
  BOOST_CHECK(returned_content == content);
}

// Test synchronous HTTP with an error status returned by the server.
void read_stream_synchronous_http_not_found_test()
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

  boost::asio::io_service io_service;
  urdl::read_stream stream1(io_service);

  boost::system::error_code ec;
  stream1.open("http://localhost:" + port + "/", ec);

  bool request_matched = server.stop();

  BOOST_CHECK(request_matched);
  BOOST_CHECK(ec == urdl::http::errc::not_found);
}

struct handler
{
  boost::system::error_code& ec_;
  std::size_t& size_;
  void operator()(const boost::system::error_code& ec, std::size_t size = 0)
  {
    ec_ = ec;
    size_ = size;
  }
};

// Test asynchronous HTTP.
void read_stream_asynchronous_http_test()
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

  boost::asio::io_service io_service;
  urdl::read_stream stream1(io_service);

  boost::system::error_code ec;
  std::size_t bytes_transferred = 0;
  handler h = { ec, bytes_transferred };

  stream1.async_open("http://localhost:" + port + "/", h);
  io_service.run();
  BOOST_CHECK(!ec);

  std::string returned_content(stream1.content_length(), 0);
  boost::asio::async_read(stream1, boost::asio::buffer(
        &returned_content[0], returned_content.size()), h);
  io_service.reset();
  io_service.run();
  BOOST_CHECK(!ec);
  BOOST_CHECK(bytes_transferred == returned_content.size());

  bool request_matched = server.stop();

  BOOST_CHECK(request_matched);
  BOOST_CHECK(stream1.content_type() == "text/plain");
  BOOST_CHECK(stream1.content_length() == 13);
  BOOST_CHECK(returned_content == content);
}

// Test asynchronous HTTP with an error status returned by the server.
void read_stream_asynchronous_http_not_found_test()
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

  boost::asio::io_service io_service;
  urdl::read_stream stream1(io_service);

  boost::system::error_code ec;
  std::size_t bytes_transferred = 0;
  handler h = { ec, bytes_transferred };

  stream1.async_open("http://localhost:" + port + "/", h);
  io_service.run();

  bool request_matched = server.stop();

  BOOST_CHECK(request_matched);
  BOOST_CHECK(ec == urdl::http::errc::not_found);
}

test_suite* init_unit_test_suite(int, char*[])
{
  test_suite* test = BOOST_TEST_SUITE("read_stream");
  test->add(BOOST_TEST_CASE(&read_stream_compile_test));
  test->add(BOOST_TEST_CASE(&read_stream_synchronous_http_test));
  test->add(BOOST_TEST_CASE(&read_stream_synchronous_http_not_found_test));
  test->add(BOOST_TEST_CASE(&read_stream_asynchronous_http_test));
  test->add(BOOST_TEST_CASE(&read_stream_asynchronous_http_not_found_test));
  return test;
}
