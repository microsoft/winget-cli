//
// istream.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef URDL_ISTREAM_HPP
#define URDL_ISTREAM_HPP

#include <istream>
#include <boost/utility/base_from_member.hpp>
#include <boost/system/error_code.hpp>
#include "urdl/istreambuf.hpp"

#include "urdl/detail/abi_prefix.hpp"

namespace urdl {

/// The class @c istream supports reading content from a specified URL.
/**
 * @par Remarks
 * The class stores an object of class @c istreambuf.
 *
 * Currently supported URL protocols are @c http, @c https and @c file.
 *
 * @par Example
 * To read the entire content of a resource located by a URL into a string:
 * @code
 * urdl::istream is("http://www.boost.org/LICENSE_1_0.txt");
 * if (is)
 * {
 *   std::string content;
 *   if (std::getline(is, content, std::char_traits<char>::eof()))
 *   {
 *     ...
 *   }
 * }
 * @endcode
 *
 * @par Requirements
 * @e Header: @c <urdl/istream.hpp> @n
 * @e Namespace: @c urdl
 */
class istream
  : private boost::base_from_member<istreambuf>,
    public std::basic_istream<char>
{
public:
  /// Constructs an object of class @c istream.
  /**
   * @par Remarks
   * Initializes the base class with @c std::basic_istream<char>(sb),
   * where sb is an object of class @c istreambuf stored within the class.
   */
  istream()
    : std::basic_istream<char>(
        &this->boost::base_from_member<istreambuf>::member)
  {
  }

  /// Constructs an object of class @c istream.
  /**
   * @param u The URL to open.
   *
   * @par Remarks
   * Initializes the base class with @c std::basic_istream<char>(sb),
   * where @c sb is an object of class @c istreambuf stored within the class. It
   * also opens @c sb by performing @c sb.open(u) and, if that fails (returns a
   * null pointer), calls @c setstate(failbit).
   */
  explicit istream(const url& u)
    : std::basic_istream<char>(
        &this->boost::base_from_member<istreambuf>::member)
  {
    if (rdbuf()->open(u) == 0)
      setstate(std::ios_base::failbit);
  }

  /// Constructs an object of class @c istream.
  /**
   * @param u The URL to open.
   *
   * @param options The options to be set on the stream.
   *
   * @par Remarks
   * Initializes the base class with @c std::basic_istream<char>(sb), where
   * @c sb is an object of class @c istreambuf stored within the class. It also
   * performs @c rdbuf()->set_options(options), then opens @c sb by performing
   * @c sb.open(u) and, if that fails (returns a null pointer), calls
   * @c setstate(failbit).
   *
   * @par Example
   * @code
   * urdl::option_set options;
   * options.set_option(urdl::http::max_redirects(1));
   * urdl::istream is("http://www.boost.org", options);
   * @endcode
   */
  explicit istream(const url& u, const option_set& options)
    : std::basic_istream<char>(
        &this->boost::base_from_member<istreambuf>::member)
  {
    rdbuf()->set_options(options);
    if (rdbuf()->open(u) == 0)
      setstate(std::ios_base::failbit);
  }

  /// Sets an option to control the behaviour of the stream.
  /**
   * @param option The option to be set on the stream.
   *
   * @par Remarks
   * Performs @c rdbuf()->set_option(option). Options are uniquely identified by
   * type.
   *
   * @par Example
   * @code
   * urdl::istream is;
   * is.set_option(urdl::http::max_redirects(1));
   * @endcode
   */
  template <typename Option>
  void set_option(const Option& option)
  {
    rdbuf()->set_option(option);
  }

  /// Sets options to control the behaviour of the stream.
  /**
   * @param options The options to be set on the stream. The options in the set
   * are added on top of any options already set on the stream.
   *
   * @par Remarks
   * Performs @c rdbuf()->set_options(options).
   *
   * @par Example
   * @code
   * urdl::istream is;
   * urdl::option_set options;
   * options.set_option(urdl::http::max_redirects(1));
   * options.set_option(urdl::ssl::verify_peer(false));
   * stream.set_options(options);
   * @endcode
   */
  void set_options(const option_set& options)
  {
    rdbuf()->set_options(options);
  }

  /// Gets the current value of an option that controls the behaviour of the
  /// stream.
  /**
   * @returns The current value of the option.
   *
   * @par Remarks
   * Returns @c rdbuf()->get_option<Option>(). Options are uniquely identified
   * by type.
   *
   * @par Example
   * @code
   * urdl::istream is;
   * urdl::http::max_redirects option
   *   = is.get_option<urdl::http::max_redirects>();
   * std::size_t value = option.value();
   * @endcode
   */
  template <typename Option>
  Option get_option() const
  {
    return rdbuf()->get_option<Option>();
  }

  /// Gets the values of all options set on the stream.
  /**
   * @returns An option set containing all options from the stream.
   *
   * @par Remarks
   * Returns @c rdbuf()->get_options().
   *
   * @par Example
   * To get the options that have been set on the stream:
   * @code
   * urdl::istream is;
   * ...
   * urdl::option_set options(is.get_options());
   * urdl::http::max_redirects option
   *   = options.get_option<urdl::http::max_redirects>();
   * std::size_t value = option.value();
   * @endcode
   */
  option_set get_options() const
  {
    return rdbuf()->get_options();
  }

  /// Determines whether the stream is open.
  /**
   * @returns @c true if the stream is open, @c false otherwise.
   *
   * @par Remarks
   * Returns @c rdbuf()->is_open().
   */
  bool is_open() const
  {
    return rdbuf()->is_open();
  }

  /// Opens the specified URL.
  /**
   * @param u The URL to open.
   *
   * @par Remarks
   * Calls @c rdbuf()->open(u). If that function does not return a null
   * pointer, calls @c clear(). Otherwise calls @c setstate(failbit) (which may
   * throw @c ios_base::failure).
   */
  void open(const url& u)
  {
    if (rdbuf()->open(u) == 0)
      setstate(std::ios_base::failbit);
    else
      clear();
  }

  /// Closes the stream.
  /**
   * @par Remarks
   * Calls @c rdbuf()->close() and, if that function returns a null
   * pointer, calls @c setstate(failbit) (which may throw @c ios_base::failure).
   */
  void close()
  {
    if (rdbuf()->close() == 0)
      setstate(std::ios_base::failbit);
  }

  /// Gets the underlying stream buffer.
  /**
   * @returns A pointer to the stream buffer contained within the class.
   */
  istreambuf* rdbuf() const
  {
    return const_cast<istreambuf*>(
        &this->boost::base_from_member<istreambuf>::member);
  }

  /// Gets the last error associated with the stream.
  /**
   * @returns An @c error_code corresponding to the last error from the stream.
   *
   * @par Remarks
   * Returns a reference to an @c error_code object representing the last
   * failure reported by an @c istreambuf function. The set of possible
   * @c error_code values and categories depends on the protocol of the URL
   * used to open the stream.
   *
   * @par Example
   * To take action given a specific error:
   * @code
   * urdl::istream is("http://somesite/page");
   * if (!is)
   * {
   *   if (is.error() == urdl::http::errc::forbidden)
   *   {
   *     std::cout << "Computer says no" << std::endl;
   *   }
   * }
   * @endcode
   */
  const boost::system::error_code& error() const
  {
    return rdbuf()->puberror();
  }

  /// Gets the open timeout of the stream.
  /**
   * @returns The timeout, in milliseconds, used when opening a URL.
   *
   * @par Remarks
   * Returns @c rdbuf()->open_timeout().
   */
  std::size_t open_timeout() const
  {
    return rdbuf()->open_timeout();
  }

  /// Sets the open timeout of the stream.
  /**
   * @param milliseconds The timeout, in milliseconds, to be used when opening
   * a URL.
   *
   * @par Remarks
   * Performs @c rdbuf()->open_timeout(milliseconds).
   */
  void open_timeout(std::size_t milliseconds)
  {
    rdbuf()->open_timeout(milliseconds);
  }

  /// Gets the read timeout of the stream.
  /**
   * @returns The timeout, in milliseconds, used for individual read operations
   * on the underlying transport.
   *
   * @par Remarks
   * Returns @c rdbuf()->read_timeout().
   */
  std::size_t read_timeout() const
  {
    return rdbuf()->read_timeout();
  }

  /// Sets the read timeout of the stream.
  /**
   * @param milliseconds The timeout, in milliseconds, to be used for individual
   * read operations on the underlying transport.
   *
   * @par Remarks
   * Performs @c rdbuf()->read_timeout(milliseconds).
   */
  void read_timeout(std::size_t milliseconds)
  {
    rdbuf()->read_timeout(milliseconds);
  }

  /// Gets the MIME type of the content obtained from the URL.
  /**
   * @returns A string specifying the MIME type. Examples of possible return
   * values include @c text/plain, @c text/html and @c image/png.
   *
   * @par Remarks
   * Returns @c rdbuf()->content_type().
   *
   * Not all URL protocols support a content type. For these protocols, this
   * function returns an empty string.
   */
  std::string content_type() const
  {
    return rdbuf()->content_type();
  }

  /// Gets the length of the content obtained from the URL.
  /**
   * @returns The length, in bytes, of the content. If the content associated
   * with the URL does not specify a length,
   * @c std::numeric_limits<std::size_t>::max().
   *
   * @par Remarks
   * Returns @c rdbuf()->content_length().
   */
  std::size_t content_length() const
  {
    return rdbuf()->content_length();
  }

  /// Gets the protocol-specific headers obtained from the URL.
  /**
   * @returns A string containing the headers returned with the content from the
   * URL. The format and interpretation of these headers is specific to the
   * protocol associated with the URL.
   *
   * @par Remarks
   * Returns @c rdbuf()->headers().
   */
  std::string headers() const
  {
    return rdbuf()->headers();
  }
};

} // namespace urdl

#include "urdl/detail/abi_suffix.hpp"

#endif // URDL_ISTREAM_HPP
