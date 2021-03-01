//
// istreambuf.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef URDL_ISTREAMBUF_HPP
#define URDL_ISTREAMBUF_HPP

#include <streambuf>
#include <boost/system/error_code.hpp>
#include "urdl/detail/config.hpp"
#include "urdl/option_set.hpp"
#include "urdl/url.hpp"

#include "urdl/detail/abi_prefix.hpp"

namespace urdl {

/// The class @c istreambuf associates the input sequence with the content from
/// a specified URL.
/**
 * @par Requirements
 * @e Header: @c <urdl/istreambuf.hpp> @n
 * @e Namespace: @c urdl
 */
class istreambuf
  : public std::streambuf
{
public:
  /// Constructs an object of class @c istreambuf.
  URDL_DECL istreambuf();

  /// Destroys an object of class @c istreambuf.
  URDL_DECL ~istreambuf();

  /// Sets an option to control the behaviour of the stream buffer.
  /**
   * @param option The option to be set on the stream buffer.
   *
   * @par Remarks
   * Options are uniquely identified by type.
   */
  template <typename Option>
  void set_option(const Option& option)
  {
    option_set options;
    options.set_option(option);
    set_options(options);
  }

  /// Sets options to control the behaviour of the stream buffer.
  /**
   * @param options The options to be set on the stream buffer.
   */
  URDL_DECL void set_options(const option_set& options);

  /// Gets the current value of an option that controls the behaviour of the
  /// stream buffer.
  /**
   * @returns The current value of the option.
   *
   * @par Remarks
   * Options are uniquely identified by type.
   */
  template <typename Option>
  Option get_option() const
  {
    option_set options(get_options());
    return options.get_option<Option>();
  }

  /// Gets the values of all options set on the stream.
  /**
   * @returns An option set containing all options from the stream buffer.
   */
  URDL_DECL option_set get_options() const;

  /// Determines whether the stream buffer is open.
  /**
   * @returns @c true if the stream buffer is open, @c false otherwise.
   *
   * @par Remarks
   * Returns @c true if a previous call to @c open succeeded (returned a
   * non-null value) and there has been no intervening call to @c close.
   */
  URDL_DECL bool is_open() const;

  /// Opens the specified URL.
  /**
   * @param u The URL to open.
   *
   * @returns @c this on success, a null pointer otherwise.
   *
   * @par Remarks
   * If <tt>is_open() != false</tt>, returns a null pointer. Otherwise,
   * initializes the @c istreambuf as required.
   */
  URDL_DECL istreambuf* open(const url& u);

  /// Closes the stream buffer.
  /**
   * @returns @c this on success, a null pointer otherwise.
   *
   * @par Remarks
   * If <tt>is_open() == false</tt>, returns a null pointer. Otherwise, closes
   * the underlying transport's resources as required. If any of those
   * operations fail, @c close fails by returning a null pointer.
   */
  URDL_DECL istreambuf* close();

  /// Gets the last error associated with the stream buffer.
  /**
   * @returns An @c error_code corresponding to the last error from the stream
   * buffer.
   *
   * @par Remarks
   * Returns @c error().
   */
  URDL_DECL const boost::system::error_code& puberror() const;

  /// Gets the open timeout of the stream buffer.
  /**
   * @returns The timeout, in milliseconds, used when opening a URL.
   */
  URDL_DECL std::size_t open_timeout() const;

  /// Sets the open timeout of the stream buffer.
  /**
   * @param milliseconds The timeout, in milliseconds, to be used when opening
   * a URL.
   */
  URDL_DECL void open_timeout(std::size_t milliseconds);

  /// Gets the read timeout of the stream buffer.
  /**
   * @returns The timeout, in milliseconds, used for individual read operations
   * on the underlying transport, when downloading the URL's content.
   */
  URDL_DECL std::size_t read_timeout() const;

  /// Sets the read timeout of the stream buffer.
  /**
   * @param milliseconds The timeout, in milliseconds, to be used for individual
   * read operations on the underlying transport, when downloading the URL's
   * content.
   */
  URDL_DECL void read_timeout(std::size_t milliseconds);

  /// Gets the MIME type of the content obtained from the URL.
  /**
   * @returns A string specifying the MIME type. Examples of possible return
   * values include @c text/plain, @c text/html and @c image/png.
   *
   * @par Remarks
   * Not all URL protocols support a content type. For these protocols, this
   * function returns an empty string.
   */
  URDL_DECL std::string content_type() const;

  /// Gets the length of the content obtained from the URL.
  /**
   * @returns The length, in bytes, of the content. If the content associated
   * with the URL does not specify a length,
   * @c std::numeric_limits<std::size_t>::max().
   */
  URDL_DECL std::size_t content_length() const;

  /// Gets the protocol-specific headers obtained from the URL.
  /**
   * @returns A string containing the headers returned with the content from the
   * URL. The format and interpretation of these headers is specific to the
   * protocol associated with the URL.
   */
  URDL_DECL std::string headers() const;

protected:
  /// Overrides @c std::streambuf behaviour.
  /**
   * par Remarks
   * Behaves according to the specification of @c std::streambuf::underflow().
   */
  URDL_DECL int_type underflow();

  /// Gets the last error associated with the stream.
  /**
   * @returns An @c error_code corresponding to the last error from the stream.
   *
   * @par Remarks
   * Returns a reference to an @c error_code object representing the last
   * failure reported by an @c istreambuf function. The set of possible
   * @c error_code values and categories depends on the protocol of the URL
   * used to open the stream buffer.
   */
  URDL_DECL virtual const boost::system::error_code& error() const;

private:
  URDL_DECL void init_buffers();

  struct body;
  body* body_;
};

} // namespace urdl

#include "urdl/detail/abi_suffix.hpp"

#if defined(URDL_HEADER_ONLY)
# include "urdl/impl/istreambuf.ipp"
#endif

#endif // URDL_ISTREAMBUF_HPP
