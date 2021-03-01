//
// read_stream.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef URDL_READ_STREAM_HPP
#define URDL_READ_STREAM_HPP

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include <boost/throw_exception.hpp>
#include "urdl/http.hpp"
#include "urdl/option_set.hpp"
#include "urdl/url.hpp"
#include "urdl/detail/coroutine.hpp"
#include "urdl/detail/file_read_stream.hpp"
#include "urdl/detail/http_read_stream.hpp"

#if !defined(URDL_DISABLE_SSL)
# include <boost/asio/ssl.hpp>
#endif // !defined(URDL_DISABLE_SSL)

#include "urdl/detail/abi_prefix.hpp"

namespace urdl {

/// The class @c read_stream supports reading content from a specified URL
/// using synchronous or asynchronous operations.
/**
 * @par Remarks
 * Currently supported URL protocols are @c http, @c https and @c file.
 *
 * The class @c read_stream meets the type requirements for @c SyncReadStream
 * and @c AsyncReadStream, as defined in the Boost.Asio documentation. This
 * allows objects of class @c read_stream to be used with the functions
 * @c boost::asio::read, @c boost::asio::async_read, @c boost::asio::read_until
 * and @c boost::asio::async_read_until.
 *
 * @par Example
 * To synchronously open the URL, read the content and write it to standard
 * output:
 * @code
 * try
 * {
 *   boost::asio::io_service io_service;
 *   urdl::read_stream read_stream(io_service);
 *   read_stream.open("http://www.boost.org/LICENSE_1_0.txt");
 *   for (;;)
 *   {
 *     char data[1024];
 *     boost::system::error_code ec;
 *     std::size_t length = stream.read_some(boost::asio::buffer(data), ec);
 *     if (ec == boost::asio::error::eof)
 *       break;
 *     if (ec)
 *       throw boost::system::system_error(ec);
 *     os.write(data, length);
 *   }
 * }
 * catch (std::exception& e)
 * {
 *   std::cerr << "Exception: " << e.what() << std::endl;
 * }
 * @endcode
 *
 * To asynchronously open the URL, read the content and write it to standard
 * output:
 * @code
 * boost::asio::io_service io_service;
 * urdl::read_stream read_stream(io_service)
 * char data[1024];
 * ...
 * read_stream.async_open("http://www.boost.org/LICENSE_1_0.txt", open_handler);
 * ...
 * void open_handler(const boost::system::error_code& ec)
 * {
 *   if (!ec)
 *   {
 *     read_stream.async_read_some(boost::asio::buffer(data), read_handler);
 *   }
 * }
 * ...
 * void read_handler(const boost::system::error_code& ec, std::size_t length)
 * {
 *   if (!ec)
 *   {
 *     std::cout.write(data, length);
 *     read_stream.async_read_some(boost::asio::buffer(data), read_handler);
 *   }
 * }
 * @endcode
 *
 * @par Requirements
 * @e Header: @c <urdl/read_stream.hpp> @n
 * @e Namespace: @c urdl
 */
class read_stream
{
public:
  /// Constructs an object of class @c read_stream.
  /**
   * @param io_service The @c io_service object that the stream will use to
   * dispatch handlers for any asynchronous operations performed on the stream.
   */
  explicit read_stream(boost::asio::io_service& io_service)
    : io_service_(io_service),
      file_(io_service, options_),
      http_(io_service, options_),
#if !defined(URDL_DISABLE_SSL)
      ssl_context_(io_service, boost::asio::ssl::context::sslv23),
      https_(io_service, options_, ssl_context_),
#endif // !defined(URDL_DISABLE_SSL)
      protocol_(unknown)
  {
#if !defined(URDL_DISABLE_SSL)
    ssl_context_.set_verify_mode(boost::asio::ssl::context::verify_peer);
    SSL_CTX_set_default_verify_paths(ssl_context_.impl());
#endif // !defined(URDL_DISABLE_SSL)
  }

  /// Gets the @c io_service associated with the stream.
  /**
   * @returns A reference to the @c io_service object that the stream will use
   * to dispatch handlers. Ownership is not transferred to the caller.
   */
  boost::asio::io_service& get_io_service()
  {
    return io_service_;
  }

  /// Sets an option to control the behaviour of the stream.
  /**
   * @param option The option to be set on the stream.
   *
   * @par Remarks
   * Options are uniquely identified by type.
   *
   * @par Example
   * @code
   * urdl::read_stream stream(io_service);
   * stream.set_option(urdl::http::max_redirects(1));
   * @endcode
   */
  template <typename Option>
  void set_option(const Option& option)
  {
    options_.set_option(option);
  }

  /// Sets options to control the behaviour of the stream.
  /**
   * @param options The options to be set on the stream. The options in the set
   * are added on top of any options already set on the stream.
   *
   * @par Example
   * @code
   * urdl::read_stream stream(io_service);
   * urdl::option_set options;
   * options.set_option(urdl::http::max_redirects(1));
   * options.set_option(urdl::ssl::verify_peer(false));
   * stream.set_options(options);
   * @endcode
   */
  void set_options(const option_set& options)
  {
    options_.set_options(options);
  }

  /// Gets the current value of an option that controls the behaviour of the
  /// stream.
  /**
   * @returns The current value of the option.
   *
   * @par Remarks
   * Options are uniquely identified by type.
   *
   * @par Example
   * @code
   * urdl::read_stream stream(io_service);
   * urdl::http::max_redirects option
   *   = stream.get_option<urdl::http::max_redirects>();
   * std::size_t value = option.value();
   * @endcode
   */
  template <typename Option>
  Option get_option() const
  {
    return options_.get_option<Option>();
  }

  /// Gets the values of all options set on the stream.
  /**
   * @returns An option set containing all options from the stream.
   *
   * @par Example
   * @code
   * urdl::read_stream stream(io_service);
   * ...
   * urdl::option_set options(stream.get_options());
   * urdl::http::max_redirects option
   *   = options.get_option<urdl::http::max_redirects>();
   * std::size_t value = option.value();
   * @endcode
   */
  option_set get_options() const
  {
    return options_;
  }

  /// Determines whether the stream is open.
  /**
   * @returns @c true if the stream is open, @c false otherwise.
   */
  bool is_open() const
  {
    switch (protocol_)
    {
    case file:
      return file_.is_open();
    case http:
      return http_.is_open();
#if !defined(URDL_DISABLE_SSL)
    case https:
      return https_.is_open();
#endif // !defined(URDL_DISABLE_SSL)
    default:
      return false;
    }
  }

  /// Opens the specified URL.
  /**
   * @param u The URL to open.
   *
   * @throws boost::system::system_error Thrown on failure.
   *
   * @par Example
   * @code
   * urdl::read_stream read_stream(io_service);
   *
   * try
   * {
   *   read_stream.open("http://www.boost.org");
   * }
   * catch (boost::system::error_code& e)
   * {
   *   std::cerr << e.what() << std::endl;
   * }
   * @endcode
   */
  void open(const url& u)
  {
    boost::system::error_code ec;
    if (open(u, ec))
    {
      boost::system::system_error ex(ec);
      boost::throw_exception(ex);
    }
  }

  /// Opens the specified URL.
  /**
   * @param u The URL to open.
   *
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns @c ec.
   *
   * @par Example
   * @code
   * urdl::read_stream read_stream(io_service);
   *
   * boost::system::error_code ec;
   * read_stream.open("http://www.boost.org", ec);
   * if (ec)
   * {
   *   std::cerr << ec.message() << std::endl;
   * }
   * @endcode
   */
  boost::system::error_code open(const url& u, boost::system::error_code& ec)
  {
    url tmp_url = u;
    std::size_t redirects = 0;
    for (;;)
    {
      if (tmp_url.protocol() == "file")
      {
        protocol_ = file;
        return file_.open(tmp_url, ec);
      }
      else if (tmp_url.protocol() == "http")
      {
        protocol_ = http;
        http_.open(tmp_url, ec);
        if (ec == http::errc::moved_permanently || ec == http::errc::found)
        {
          std::size_t max_redirects = options_.get_option<
              urdl::http::max_redirects>().value();
          if (redirects < max_redirects)
          {
            ++redirects;
            tmp_url = http_.location();
            http_.close(ec);
            continue;
          }
        }
        return ec;
      }
#if !defined(URDL_DISABLE_SSL)
      else if (tmp_url.protocol() == "https")
      {
        protocol_ = https;
        https_.open(tmp_url, ec);
        if (ec == http::errc::moved_permanently || ec == http::errc::found)
        {
          std::size_t max_redirects = options_.get_option<
              urdl::http::max_redirects>().value();
          if (redirects < max_redirects)
          {
            ++redirects;
            tmp_url = https_.location();
            https_.close(ec);
            continue;
          }
        }
        return ec;
      }
#endif // !defined(URDL_DISABLE_SSL)
      else
      {
        ec = boost::asio::error::operation_not_supported;
        return ec;
      }
    }
  }

  /// Asynchronously opens the specified URL.
  /**
   * @param u The URL to open.
   *
   * @param handler The handler to be called when the open operation completes.
   * Copies will be made of the handler as required. The function signature of
   * the handler must be:
   * @code
   * void handler(
   *   const boost::system::error_code& ec // Result of operation.
   * );
   * @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the handler will not be invoked from within this function. Invocation
   * of the handler will be performed in a manner equivalent to using
   * @c boost::asio::io_service::post().
   *
   * @par Example
   * @code
   * void open_handler(const boost::system::error_code& ec)
   * {
   *   if (!ec)
   *   {
   *     // Open succeeded.
   *   }
   * }
   * ...
   * urdl::read_stream read_stream(io_service);
   * read_stream.async_open("http://www.boost.org/", open_handler);
   * @endcode
   */
  template <typename Handler>
  URDL_INITFN_RESULT_TYPE(Handler,
      void (boost::system::error_code))
  async_open(const url& u, Handler handler)
  {
#if (BOOST_VERSION >= 105400)
    typedef typename boost::asio::handler_type<Handler,
      void (boost::system::error_code)>::type real_handler_type;
    real_handler_type real_handler(handler);
    boost::asio::async_result<real_handler_type> result(real_handler);
#else // (BOOST_VERSION >= 105400)
    typedef Handler real_handler_type;
    Handler real_handler(handler);
#endif // (BOOST_VERSION >= 105400)

    open_coro<real_handler_type>(this, u, real_handler)(
        boost::system::error_code());

#if (BOOST_VERSION >= 105400)
    return result.get();
#endif // (BOOST_VERSION >= 105400)
  }

  /// Closes the stream.
  /**
   * @throws asio::system_error Thrown on failure.
   *
   * @par Remarks
   * Any asynchronous open or read operations will be cancelled, and will
   * complete with the @c boost::asio::error::operation_aborted error.
   */
  void close()
  {
    boost::system::error_code ec;
    if (close(ec))
    {
      boost::system::system_error ex(ec);
      boost::throw_exception(ex);
    }
  }

  /// Closes the stream.
  /**
   * @param ec Set to indicate what error occurred, if any.
   *
   * @returns @c ec.
   *
   * @par Remarks
   * Any asynchronous open or read operations will be cancelled, and will
   * complete with the @c boost::asio::error::operation_aborted error.
   */
  boost::system::error_code close(boost::system::error_code& ec)
  {
    switch (protocol_)
    {
    case file:
      return file_.close(ec);
    case http:
      return http_.close(ec);
#if !defined(URDL_DISABLE_SSL)
    case https:
      return https_.close(ec);
#endif // !defined(URDL_DISABLE_SSL)
    default:
      ec = boost::system::error_code();
      break;
    }

    return ec;
  }

  /// Gets the MIME type of the content obtained from the URL.
  /**
   * @returns A string specifying the MIME type. Examples of possible return
   * values include @c text/plain, @c text/html and @c image/png.
   *
   * @par Remarks
   * Not all URL protocols support a content type. For these protocols, this
   * function returns an empty string.
   */
  std::string content_type() const
  {
    switch (protocol_)
    {
    case file:
      return std::string();
    case http:
      return http_.content_type();
#if !defined(URDL_DISABLE_SSL)
    case https:
      return https_.content_type();
#endif // !defined(URDL_DISABLE_SSL)
    default:
      return std::string();
    }
  }

  /// Gets the length of the content obtained from the URL.
  /**
   * @returns The length, in bytes, of the content. If the content associated
   * with the URL does not specify a length,
   * @c std::numeric_limits<std::size_t>::max().
   */
  std::size_t content_length() const
  {
    switch (protocol_)
    {
    case file:
      return ~std::size_t(0);
    case http:
      return http_.content_length();
#if !defined(URDL_DISABLE_SSL)
    case https:
      return https_.content_length();
#endif // !defined(URDL_DISABLE_SSL)
    default:
      return ~std::size_t(0);
    }
  }

  /// Gets the protocol-specific headers obtained from the URL.
  /**
   * @returns A string containing the headers returned with the content from the
   * URL. The format and interpretation of these headers is specific to the
   * protocol associated with the URL.
   */
  std::string headers() const
  {
    switch (protocol_)
    {
    case file:
      return std::string();
    case http:
      return http_.headers();
#if !defined(URDL_DISABLE_SSL)
    case https:
      return https_.headers();
#endif // !defined(URDL_DISABLE_SSL)
    default:
      return std::string();
    }
  }

  /// Reads some data from the stream.
  /**
   * @param buffers One or more buffers into which the data will be read. The
   * type must meet the requirements for @c MutableBufferSequence, as defined in
   * the Boost.Asio documentation.
   *
   * @returns The number of bytes read.
   *
   * @throws boost::system::system_error Thrown on failure. An error code of
   * @c boost::asio::error::eof indicates that the end of the URL content has
   * been reached.
   *
   * @par Remarks
   * The function call will block until one or more bytes of data has been read
   * successfully, or until an error occurs.
   *
   * The @c read_some operation may not read all of the requested number of
   * bytes. Consider using the @c boost::asio::read function if you need to
   * ensure that the requested amount of data is read before the blocking
   * operation completes.
   *
   * @par Example
   * To read into a single data buffer use the @c boost::asio::buffer function
   * as follows:
   * @code
   * read_stream.read_some(boost::asio::buffer(data, size));
   * @endcode
   * See the documentation for the @c boost::asio::buffer function for
   * information on reading into multiple buffers in one go, and how to use it
   * with arrays, @c boost::array or @c std::vector.
   */
  template <typename MutableBufferSequence>
  std::size_t read_some(const MutableBufferSequence& buffers)
  {
    boost::system::error_code ec;
    std::size_t bytes_transferred = read_some(buffers, ec);
    if (ec)
    {
      boost::system::system_error ex(ec);
      boost::throw_exception(ex);
    }
    return bytes_transferred;
  }

  /// Reads some data from the stream.
  /**
   * @param buffers One or more buffers into which the data will be read. The
   * type must meet the requirements for @c MutableBufferSequence, as defined in
   * the Boost.Asio documentation.
   *
   * @param ec Set to indicate what error occurred, if any. An error code of
   * @c boost::asio::error::eof indicates that the end of the URL content has
   * been reached.
   *
   * @returns The number of bytes read.
   *
   * @par Remarks
   * This function is used to read data from the stream. The function call will
   * block until one or more bytes of data has been read successfully, or until
   * an error occurs.
   *
   * The @c read_some operation may not read all of the requested number of
   * bytes. Consider using the @c boost::asio::read function if you need to
   * ensure that the requested amount of data is read before the blocking
   * operation completes.
   *
   * @par Example
   * To read into a single data buffer use the @c boost::asio::buffer function
   * as follows:
   * @code
   * read_stream.read_some(boost::asio::buffer(data, size));
   * @endcode
   * See the documentation for the @c boost::asio::buffer function for
   * information on reading into multiple buffers in one go, and how to use it
   * with arrays, @c boost::array or @c std::vector.
   */
  template <typename MutableBufferSequence>
  std::size_t read_some(const MutableBufferSequence& buffers,
      boost::system::error_code& ec)
  {
    switch (protocol_)
    {
    case file:
      return file_.read_some(buffers, ec);
    case http:
      return http_.read_some(buffers, ec);
#if !defined(URDL_DISABLE_SSL)
    case https:
      return https_.read_some(buffers, ec);
#endif // !defined(URDL_DISABLE_SSL)
    default:
      ec = boost::asio::error::operation_not_supported;
      return 0;
    }
  }

  /// Asynchronously reads some data from the stream.
  /**
   * @param buffers One or more buffers into which the data will be read. The
   * type must meet the requirements for @c MutableBufferSequence, as defined in
   * the Boost.Asio documentation. Although the buffers object may be copied as
   * necessary, ownership of the underlying memory blocks is retained by the
   * caller, which must guarantee that they remain valid until the handler is
   * called.
   *
   * @param handler The handler to be called when the read operation completes.
   * Copies will be made of the handler as required. The function signature of
   * the handler must be:
   * @code
   * void handler(
   *   const boost::system::error_code& ec, // Result of operation.
   *   std::size_t bytes_transferred        // Number of bytes read.
   * );
   * @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the handler will not be invoked from within this function. Invocation
   * of the handler will be performed in a manner equivalent to using
   * @c boost::asio::io_service::post().
   *
   * @par Remarks
   * The asynchronous operation will continue until one or more bytes of data
   * has been read successfully, or until an error occurs.
   *
   * The @c async_read_some operation may not read all of the requested number
   * of bytes. Consider using the @c boost::asio::async_read function if you
   * need to ensure that the requested amount of data is read before the
   * asynchronous operation completes.
   *
   * @par Example
   * To read into a single data buffer use the @c boost::asio::buffer function
   * as follows:
   * @code
   * read_stream.async_read_some(boost::asio::buffer(data, size), handler);
   * @endcode
   * See the documentation for the @c boost::asio::buffer function for
   * information on reading into multiple buffers in one go, and how to use it
   * with arrays, @c boost::array or @c std::vector.
   */
  template <typename MutableBufferSequence, typename Handler>
  URDL_INITFN_RESULT_TYPE(Handler,
      void (boost::system::error_code, std::size_t))
  async_read_some(const MutableBufferSequence& buffers, Handler handler)
  {
#if (BOOST_VERSION >= 105400)
    typedef typename boost::asio::handler_type<Handler,
      void (boost::system::error_code, std::size_t)>::type real_handler_type;
    real_handler_type real_handler(handler);
    boost::asio::async_result<real_handler_type> result(real_handler);
#else // (BOOST_VERSION >= 105400)
    Handler real_handler(handler);
#endif // (BOOST_VERSION >= 105400)

    switch (protocol_)
    {
    case file:
      file_.async_read_some(buffers, real_handler);
      break;
    case http:
      http_.async_read_some(buffers, real_handler);
      break;
#if !defined(URDL_DISABLE_SSL)
    case https:
      https_.async_read_some(buffers, real_handler);
      break;
#endif // !defined(URDL_DISABLE_SSL)
    default:
      boost::system::error_code ec
        = boost::asio::error::operation_not_supported;
      io_service_.post(boost::asio::detail::bind_handler(real_handler, ec, 0));
      break;
    }

#if (BOOST_VERSION >= 105400)
    return result.get();
#endif // (BOOST_VERSION >= 105400)
  }

private:
  template <typename Handler>
  class open_coro : detail::coroutine
  {
  public:
    open_coro(read_stream* this_ptr, const url& u, Handler handler)
      : this_(this_ptr),
        url_(u),
        handler_(handler)
    {
    }

    void operator()(boost::system::error_code ec)
    {
      URDL_CORO_BEGIN;

      for (;;)
      {
        if (url_.protocol() == "file")
        {
          this_->protocol_ = file;
          URDL_CORO_YIELD(this_->file_.async_open(url_, *this));
          handler_(ec);
          return;
        }
        else if (url_.protocol() == "http")
        {
          this_->protocol_ = http;
          URDL_CORO_YIELD(this_->http_.async_open(url_, *this));
          if (ec == http::errc::moved_permanently || ec == http::errc::found)
          {
            url_ = this_->http_.location();
            this_->http_.close(ec);
            continue;
          }
          handler_(ec);
          return;
        }
#if !defined(URDL_DISABLE_SSL)
        else if (url_.protocol() == "https")
        {
          this_->protocol_ = https;
          URDL_CORO_YIELD(this_->https_.async_open(url_, *this));
          if (ec == http::errc::moved_permanently || ec == http::errc::found)
          {
            url_ = this_->https_.location();
            this_->https_.close(ec);
            continue;
          }
          handler_(ec);
          return;
        }
#endif // !defined(URDL_DISABLE_SSL)
        else
        {
          ec = boost::asio::error::operation_not_supported;
          this_->io_service_.post(
              boost::asio::detail::bind_handler(handler_, ec));
          return;
        }
      }

      URDL_CORO_END;
    }

    friend void* asio_handler_allocate(std::size_t size,
        open_coro<Handler>* this_handler)
    {
      using boost::asio::asio_handler_allocate;
      return asio_handler_allocate(size, &this_handler->handler_);
    }

    friend void asio_handler_deallocate(void* pointer, std::size_t size,
        open_coro<Handler>* this_handler)
    {
      using boost::asio::asio_handler_deallocate;
      asio_handler_deallocate(pointer, size, &this_handler->handler_);
    }

    template <typename Function>
    friend void asio_handler_invoke(const Function& function,
        open_coro<Handler>* this_handler)
    {
      using boost::asio::asio_handler_invoke;
      asio_handler_invoke(function, &this_handler->handler_);
    }

  private:
    read_stream* this_;
    url url_;
    Handler handler_;
  };

  template <typename Handler> friend class open_coro;

  boost::asio::io_service& io_service_;
  option_set options_;
  detail::file_read_stream file_;
  detail::http_read_stream<boost::asio::ip::tcp::socket> http_;
#if !defined(URDL_DISABLE_SSL)
  boost::asio::ssl::context ssl_context_;
  detail::http_read_stream<
      boost::asio::ssl::stream<
        boost::asio::ip::tcp::socket> > https_;
#endif // !defined(URDL_DISABLE_SSL)
  enum { unknown, file, http, https } protocol_;
};

} // namespace urdl

#include "urdl/detail/abi_suffix.hpp"

#endif // URDL_READ_STREAM_HPP
