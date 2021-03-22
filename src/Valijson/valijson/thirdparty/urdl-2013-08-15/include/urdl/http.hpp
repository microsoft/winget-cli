//
// http.hpp
// ~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef URDL_HTTP_HPP
#define URDL_HTTP_HPP

#include <string>
#include <boost/system/error_code.hpp>
#include "urdl/detail/config.hpp"

#include "urdl/detail/abi_prefix.hpp"

namespace urdl {
namespace http {

/// Gets the error category for HTTP errors.
/**
 * @returns The @c boost::system::error_category used for HTTP errors.
 *
 * @par Requirements
 * @e Header: @c <urdl/http.hpp> @n
 * @e Namespace: @c urdl::http
 */
URDL_DECL const boost::system::error_category& error_category();

/// Option to specify the HTTP request method.
/**
 * @par Remarks
 * The default request method is "GET".
 *
 * @par Example
 * To set the request method for an object of class @c urdl::istream:
 * @code
 * urdl::istream is;
 * is.set_option(urdl::http::request_method("HEAD"));
 * is.open("http://www.boost.org");
 * @endcode
 *
 * To set the request method for an object of class @c urdl::read_stream:
 * @code
 * urdl::read_stream stream;
 * stream.set_option(urdl::http::request_method("HEAD"));
 * stream.open("http://www.boost.org");
 * @endcode
 *
 * @par Requirements
 * @e Header: @c <urdl/http.hpp> @n
 * @e Namespace: @c urdl::http
 */
class request_method
{
public:
  /// Constructs an object of class @c request_method.
  /**
   * @par Remarks
   * Postcondition: <tt>value() == "GET"</tt>.
   */
  request_method()
    : value_("GET")
  {
  }

  /// Constructs an object of class @c request_method.
  /**
   * @param v The desired value for the option.
   *
   * @par Remarks
   * Postcondition: <tt>value() == v</tt>
   */
  explicit request_method(const std::string& v)
    : value_(v)
  {
  }

  /// Gets the value of the option.
  /**
   * @returns The value of the option.
   */
  std::string value() const
  {
    return value_;
  }

  /// Sets the value of the option.
  /**
   * @param v The desired value for the option.
   *
   * @par Remarks
   * Postcondition: <tt>value() == v</tt>
   */
  void value(const std::string& v)
  {
    value_ = v;
  }

private:
  std::string value_;
};

/// Option to specify content to accompany an HTTP request.
/**
 * @par Remarks
 * The default is for no content to be sent.
 *
 * @par Example
 * To add content to the HTTP request using an object of class @c urdl::istream:
 * @code
 * urdl::istream is;
 * is.set_option(urdl::http::request_method("POST"));
 * is.set_option(urdl::http::request_content("Hello, world!"));
 * is.set_option(urdl::http::request_content_type("text/plain"));
 * is.open("http://host/path");
 * @endcode
 *
 * To add content to the HTTP request using an object of class
 * @c urdl::read_stream:
 * @code
 * urdl::read_stream stream;
 * stream.set_option(urdl::http::request_method("POST"));
 * stream.set_option(urdl::http::request_content("Hello, world!"));
 * stream.set_option(urdl::http::request_content_type("text/plain"));
 * stream.open("http://host/path");
 * @endcode
 *
 * @par Requirements
 * @e Header: @c <urdl/http.hpp> @n
 * @e Namespace: @c urdl::http
 */
class request_content
{
public:
  /// Constructs an object of class @c request_content.
  /**
   * @par Remarks
   * Postcondition: <tt>value() == ""</tt>.
   */
  request_content()
    : value_("")
  {
  }

  /// Constructs an object of class @c request_content.
  /**
   * @param v The desired value for the option.
   *
   * @par Remarks
   * Postcondition: <tt>value() == v</tt>
   */
  explicit request_content(const std::string& v)
    : value_(v)
  {
  }

  /// Gets the value of the option.
  /**
   * @returns The value of the option.
   */
  std::string value() const
  {
    return value_;
  }

  /// Sets the value of the option.
  /**
   * @param v The desired value for the option.
   *
   * @par Remarks
   * Postcondition: <tt>value() == v</tt>
   */
  void value(const std::string& v)
  {
    value_ = v;
  }

private:
  std::string value_;
};

/// Option to specify the type of the content that accompanies an HTTP request.
/**
 * @par Remarks
 * The default is for no content type to be specified in the request.
 *
 * @par Example
 * To add content to the HTTP request using an object of class @c urdl::istream:
 * @code
 * urdl::istream is;
 * is.set_option(urdl::http::request_method("POST"));
 * is.set_option(urdl::http::request_content("Hello, world!"));
 * is.set_option(urdl::http::request_content_type("text/plain"));
 * is.open("http://host/path");
 * @endcode
 *
 * To add content to the HTTP request using an object of class
 * @c urdl::read_stream:
 * @code
 * urdl::read_stream stream;
 * stream.set_option(urdl::http::request_method("POST"));
 * stream.set_option(urdl::http::request_content("Hello, world!"));
 * stream.set_option(urdl::http::request_content_type("text/plain"));
 * stream.open("http://host/path");
 * @endcode
 *
 * @par Requirements
 * @e Header: @c <urdl/http.hpp> @n
 * @e Namespace: @c urdl::http
 */
class request_content_type
{
public:
  /// Constructs an object of class @c request_content_type.
  /**
   * @par Remarks
   * Postcondition: <tt>value() == ""</tt>.
   */
  request_content_type()
    : value_("")
  {
  }

  /// Constructs an object of class @c request_content_type.
  /**
   * @param v The desired value for the option.
   *
   * @par Remarks
   * Postcondition: <tt>value() == v</tt>
   */
  explicit request_content_type(const std::string& v)
    : value_(v)
  {
  }

  /// Gets the value of the option.
  /**
   * @returns The value of the option.
   */
  std::string value() const
  {
    return value_;
  }

  /// Sets the value of the option.
  /**
   * @param v The desired value for the option.
   *
   * @par Remarks
   * Postcondition: <tt>value() == v</tt>
   */
  void value(const std::string& v)
  {
    value_ = v;
  }

private:
  std::string value_;
};

/// Option to specify the maximum number of allowed HTTP redirects.
/**
 * @par Remarks
 * The default value is for there to be no limit on the number of allowed
 * redirects. Set the option to 0 to disable HTTP redirects.
 *
 * @par Example
 * To set maximum number of redirects for an object of class @c urdl::istream:
 * @code
 * urdl::istream is;
 * is.set_option(urdl::http::max_redirects(1));
 * is.open("http://www.boost.org");
 * @endcode
 *
 * To set maximum number of redirects for an object of class
 * @c urdl::read_stream:
 * @code
 * urdl::read_stream stream;
 * stream.set_option(urdl::http::max_redirects(1));
 * stream.open("http://www.boost.org");
 * @endcode
 *
 * @par Requirements
 * @e Header: @c <urdl/http.hpp> @n
 * @e Namespace: @c urdl::http
 */
class max_redirects
{
public:
  /// Constructs an object of class @c max_redirects.
  /**
   * @par Remarks
   * Postcondition: <tt>value() == std::numeric_limits<std::size_t>::max()</tt>.
   */
  max_redirects()
    : value_(~std::size_t(0))
  {
  }

  /// Constructs an object of class @c max_redirects.
  /**
   * @param v The desired value for the option.
   *
   * @par Remarks
   * Postcondition: <tt>value() == v</tt>
   */
  explicit max_redirects(std::size_t v)
    : value_(v)
  {
  }

  /// Gets the value of the option.
  /**
   * @returns The value of the option.
   */
  std::size_t value() const
  {
    return value_;
  }

  /// Sets the value of the option.
  /**
   * @param v The desired value for the option.
   *
   * @par Remarks
   * Postcondition: <tt>value() == v</tt>
   */
  void value(std::size_t v)
  {
    value_ = v;
  }

private:
  std::size_t value_;
};

/// Option to specify the user agent identifier.
/**
 * @par Remarks
 * The default is to not specify the user agent.
 *
 * @par Example
 * To set the user agent for an object of class @c urdl::istream:
 * @code
 * urdl::istream is;
 * is.set_option(urdl::http::user_agent("Urdl"));
 * is.open("http://www.boost.org");
 * @endcode
 *
 * To set the user agent for an object of class @c urdl::read_stream:
 * @code
 * urdl::read_stream stream;
 * stream.set_option(urdl::http::user_agent("Urdl"));
 * stream.open("http://www.boost.org");
 * @endcode
 *
 * @par Requirements
 * @e Header: @c <urdl/http.hpp> @n
 * @e Namespace: @c urdl::http
 */
class user_agent
{
public:
  /// Constructs an object of class @c user_agent.
  /**
   * @par Remarks
   * Postcondition: <tt>value() == ""</tt>.
   */
  user_agent()
    : value_("")
  {
  }

  /// Constructs an object of class @c user_agent.
  /**
   * @param v The desired value for the option.
   *
   * @par Remarks
   * Postcondition: <tt>value() == v</tt>
   */
  explicit user_agent(const std::string& v)
    : value_(v)
  {
  }

  /// Gets the value of the option.
  /**
   * @returns The value of the option.
   */
  std::string value() const
  {
    return value_;
  }

  /// Sets the value of the option.
  /**
   * @param v The desired value for the option.
   *
   * @par Remarks
   * Postcondition: <tt>value() == v</tt>
   */
  void value(const std::string& v)
  {
    value_ = v;
  }

private:
  std::string value_;
};

namespace errc {

/// HTTP error codes.
/**
 * The enumerators of type @c errc_t are implicitly convertible to objects of
 * type @c boost::system::error_code.
 *
 * @par Requirements
 * @e Header: @c <urdl/http.hpp> @n
 * @e Namespace: @c urdl::http
 */
enum errc_t
{
  // Client-generated errors.

  /// The response's status line was malformed.
  malformed_status_line = 1,

  /// The response's headers were malformed.
  malformed_response_headers = 2,

  // Server-generated status codes.

  /// The server-generated status code "100 Continue".
  continue_request = 100,

  /// The server-generated status code "101 Switching Protocols".
  switching_protocols = 101,

  /// The server-generated status code "200 OK".
  ok = 200,

  /// The server-generated status code "201 Created".
  created = 201,

  /// The server-generated status code "202 Accepted".
  accepted = 202,

  /// The server-generated status code "203 Non-Authoritative Information".
  non_authoritative_information = 203,

  /// The server-generated status code "204 No Content".
  no_content = 204,

  /// The server-generated status code "205 Reset Content".
  reset_content = 205,

  /// The server-generated status code "206 Partial Content".
  partial_content = 206,

  /// The server-generated status code "300 Multiple Choices".
  multiple_choices = 300,

  /// The server-generated status code "301 Moved Permanently".
  moved_permanently = 301,

  /// The server-generated status code "302 Found".
  found = 302,

  /// The server-generated status code "303 See Other".
  see_other = 303,

  /// The server-generated status code "304 Not Modified".
  not_modified = 304,

  /// The server-generated status code "305 Use Proxy".
  use_proxy = 305,

  /// The server-generated status code "307 Temporary Redirect".
  temporary_redirect = 307,

  /// The server-generated status code "400 Bad Request".
  bad_request = 400,

  /// The server-generated status code "401 Unauthorized".
  unauthorized = 401,

  /// The server-generated status code "402 Payment Required".
  payment_required = 402,

  /// The server-generated status code "403 Forbidden".
  forbidden = 403,

  /// The server-generated status code "404 Not Found".
  not_found = 404,

  /// The server-generated status code "405 Method Not Allowed".
  method_not_allowed = 405,

  /// The server-generated status code "406 Not Acceptable".
  not_acceptable = 406,

  /// The server-generated status code "407 Proxy Authentication Required".
  proxy_authentication_required = 407,

  /// The server-generated status code "408 Request Time-out".
  request_timeout = 408,

  /// The server-generated status code "409 Conflict".
  conflict = 409,

  /// The server-generated status code "410 Gone".
  gone = 410,

  /// The server-generated status code "411 Length Required".
  length_required = 411,

  /// The server-generated status code "412 Precondition Failed".
  precondition_failed = 412,

  /// The server-generated status code "413 Request Entity Too Large".
  request_entity_too_large = 413,

  /// The server-generated status code "414 Request URI Too Large".
  request_uri_too_large = 414,

  /// The server-generated status code "415 Unsupported Media Type".
  unsupported_media_type = 415,

  /// The server-generated status code "416 Requested Range Not Satisfiable".
  requested_range_not_satisfiable = 416,

  /// The server-generated status code "417 Expectation Failed".
  expectation_failed = 417,

  /// The server-generated status code "500 Internal Server Error".
  internal_server_error = 500,

  /// The server-generated status code "501 Not Implemented".
  not_implemented = 501,

  /// The server-generated status code "502 Bad Gateway".
  bad_gateway = 502,

  /// The server-generated status code "503 Service Unavailable".
  service_unavailable = 503,

  /// The server-generated status code "504 Gateway Timeout".
  gateway_timeout = 504,

  /// The server-generated status code "505 HTTP Version Not Supported".
  version_not_supported = 505
};

/// Converts a value of type @c errc_t to a corresponding object of type
/// @c boost::system::error_code.
/**
 * @par Requirements
 * @e Header: @c <urdl/http.hpp> @n
 * @e Namespace: @c urdl::http
 */
inline boost::system::error_code make_error_code(errc_t e)
{
  return boost::system::error_code(
      static_cast<int>(e), http::error_category());
}

} // namespace errc
} // namespace http
} // namespace urdl

namespace boost {
namespace system {

template <>
struct is_error_code_enum<urdl::http::errc::errc_t>
{
  static const bool value = true;
};

} // namespace system
} // namespace boost

#include "urdl/detail/abi_suffix.hpp"

#if defined(URDL_HEADER_ONLY)
# include "urdl/impl/http.ipp"
#endif

#endif // URDL_HTTP_HPP
