//
// http_read_stream.hpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef URDL_DETAIL_HTTP_READ_STREAM_HPP
#define URDL_DETAIL_HTTP_READ_STREAM_HPP

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include <algorithm>
#include <ostream>
#include <iterator>
#include "urdl/http.hpp"
#include "urdl/option_set.hpp"
#include "urdl/url.hpp"
#include "urdl/detail/connect.hpp"
#include "urdl/detail/coroutine.hpp"
#include "urdl/detail/handshake.hpp"
#include "urdl/detail/parsers.hpp"

#include "urdl/detail/abi_prefix.hpp"

namespace urdl {
namespace detail {

template <typename Stream>
class http_read_stream
{
public:
  explicit http_read_stream(boost::asio::io_service& io_service,
      option_set& options)
    : resolver_(io_service),
      socket_(io_service),
      options_(options),
      content_length_(0)
  {
  }

  template <typename Arg>
  http_read_stream(boost::asio::io_service& io_service,
      option_set& options, Arg& arg)
    : resolver_(io_service),
      socket_(io_service, arg),
      options_(options),
      content_length_(0)
  {
  }

  boost::system::error_code open(const url& u, boost::system::error_code& ec)
  {
    // Fail if the socket is already open.
    if (socket_.lowest_layer().is_open())
    {
      ec = boost::asio::error::already_open;
      return ec;
    }

    // Establish a connection to the HTTP server.
    connect(socket_.lowest_layer(), resolver_, u, ec);
    if (ec)
      return ec;

    // Perform SSL handshake if required.
    handshake(socket_, u.host(), ec);
    if (ec)
      return ec;

    // Get the HTTP options used to build the request.
    std::string request_method
      = options_.get_option<urdl::http::request_method>().value();
    std::string request_content
      = options_.get_option<urdl::http::request_content>().value();
    std::string request_content_type
      = options_.get_option<urdl::http::request_content_type>().value();
    std::string user_agent
      = options_.get_option<urdl::http::user_agent>().value();

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    std::ostream request_stream(&request_buffer_);
    request_stream << request_method << " ";
    request_stream << u.to_string(url::path_component | url::query_component);
    request_stream << " HTTP/1.0\r\n";
    request_stream << "Host: ";
    request_stream << u.to_string(url::host_component | url::port_component);
    request_stream << "\r\n";
    request_stream << "Accept: */*\r\n";
    if (request_content.length())
    {
      request_stream << "Content-Length: ";
      request_stream << request_content.length() << "\r\n";
      if (request_content_type.length())
      {
        request_stream << "Content-Type: ";
        request_stream << request_content_type << "\r\n";
      }
    }
    if (user_agent.length())
      request_stream << "User-Agent: " << user_agent << "\r\n";
    request_stream << "Connection: close\r\n\r\n";
    request_stream << request_content;

    // Send the request.
    boost::asio::write(socket_, request_buffer_,
        boost::asio::transfer_all(), ec);
    if (ec)
      return ec;

    int status_code = 0;
    for (;;)
    {
      // Read the reply status line.
      boost::asio::read_until(socket_, reply_buffer_, "\r\n", ec);
      if (ec)
        return ec;

      // Extract the response code from the status line.
      int version_major = 0;
      int version_minor = 0;
      if (!parse_http_status_line(
            std::istreambuf_iterator<char>(&reply_buffer_),
            std::istreambuf_iterator<char>(),
            version_major, version_minor, status_code))
      {
        ec = http::errc::malformed_status_line;
        return ec;
      }

      // A "continue" header means we need to keep waiting.
      if (status_code != http::errc::continue_request)
        break;
    }

    // Read list of headers and save them. If there's anything left in the reply
    // buffer afterwards, it's the start of the content returned by the HTTP
    // server.
    std::size_t bytes_transferred = boost::asio::read_until(
        socket_, reply_buffer_, "\r\n\r\n", ec);
    headers_.resize(bytes_transferred);
    reply_buffer_.sgetn(&headers_[0], bytes_transferred);
    if (ec)
      return ec;

    // Parse the headers to get Content-Type and Content-Length.
    if (!parse_http_headers(headers_.begin(), headers_.end(),
          content_type_, content_length_, location_))
    {
      ec = http::errc::malformed_response_headers;
      return ec;
    }

    // Check the response code to see if we got the page correctly.
    if (status_code != http::errc::ok)
      ec = make_error_code(static_cast<http::errc::errc_t>(status_code));

    return ec;
  }

  template <typename Handler>
  class open_coro : coroutine
  {
  public:
    open_coro(Handler handler, boost::asio::ip::tcp::resolver& resolver,
        Stream& socket, const option_set& options,
        boost::asio::streambuf& request_buffer,
        boost::asio::streambuf& reply_buffer, const url& u,
        std::string& headers, std::string& content_type,
        std::size_t& content_length, std::string& location)
      : handler_(handler),
        resolver_(resolver),
        socket_(socket),
        options_(options),
        request_buffer_(request_buffer),
        reply_buffer_(reply_buffer),
        url_(u),
        headers_(headers),
        status_code_(0),
        content_type_(content_type),
        content_length_(content_length),
        location_(location)
    {
    }

    void operator()(boost::system::error_code ec,
        std::size_t bytes_transferred = 0)
    {
      URDL_CORO_BEGIN;

      // Fail if the socket is already open.
      if (socket_.lowest_layer().is_open())
      {
        ec = boost::asio::error::already_open;
        URDL_CORO_YIELD(socket_.get_io_service().post(
              boost::asio::detail::bind_handler(*this, ec)));
        handler_(ec);
        return;
      }

      // Establish a connection to the HTTP server.
      URDL_CORO_YIELD(async_connect(socket_.lowest_layer(),
            resolver_, url_, *this));
      if (ec)
      {
        handler_(ec);
        return;
      }

      // Perform SSL handshake if required.
      URDL_CORO_YIELD(async_handshake(socket_, url_.host(), *this));
      if (ec)
      {
        handler_(ec);
        return;
      }

      {
        // Get the HTTP options used to build the request.
        std::string request_method
          = options_.get_option<urdl::http::request_method>().value();
        std::string request_content
          = options_.get_option<urdl::http::request_content>().value();
        std::string request_content_type
          = options_.get_option<urdl::http::request_content_type>().value();
        std::string user_agent
          = options_.get_option<urdl::http::user_agent>().value();

        // Form the request. We specify the "Connection: close" header so that
        // the server will close the socket after transmitting the response.
        // This will allow us to treat all data up until the EOF as the
        // content.
        std::ostream request_stream(&request_buffer_);
        request_stream << request_method << " ";
        request_stream << url_.to_string(
            url::path_component | url::query_component);
        request_stream << " HTTP/1.0\r\n";
        request_stream << "Host: ";
        request_stream << url_.to_string(
            url::host_component | url::port_component);
        request_stream << "\r\n";
        request_stream << "Accept: */*\r\n";
        if (request_content.length())
        {
          request_stream << "Content-Length: ";
          request_stream << request_content.length() << "\r\n";
          if (request_content_type.length())
          {
            request_stream << "Content-Type: ";
            request_stream << request_content_type << "\r\n";
          }
        }
        if (user_agent.length())
          request_stream << "User-Agent: " << user_agent << "\r\n";
        request_stream << "Connection: close\r\n\r\n";
        request_stream << request_content;
      }

      // Send the request.
      URDL_CORO_YIELD(boost::asio::async_write(socket_,
            request_buffer_, boost::asio::transfer_all(), *this));
      if (ec)
      {
        handler_(ec);
        return;
      }

      for (;;)
      {
        // Read the reply status line.
        URDL_CORO_YIELD(boost::asio::async_read_until(socket_,
              reply_buffer_, "\r\n", *this));
        if (ec)
        {
          handler_(ec);
          return;
        }

        // Check the response code to see if we got the page correctly.
        {
          int version_major = 0;
          int version_minor = 0;
          if (!parse_http_status_line(
                std::istreambuf_iterator<char>(&reply_buffer_),
                std::istreambuf_iterator<char>(),
                version_major, version_minor, status_code_))
          {
            ec = http::errc::malformed_status_line;
            handler_(ec);
            return;
          }
        }

        // A "continue" header means we need to keep waiting.
        if (status_code_ != http::errc::continue_request)
          break;
      }

      // Read list of headers and save them. If there's anything left in the
      // reply buffer afterwards, it's the start of the content returned by the
      // HTTP server.
      URDL_CORO_YIELD(boost::asio::async_read_until(socket_,
            reply_buffer_, "\r\n\r\n", *this));
      headers_.resize(bytes_transferred);
      reply_buffer_.sgetn(&headers_[0], bytes_transferred);
      if (ec)
      {
        handler_(ec);
        return;
      }

      // Parse the headers to get Content-Type and Content-Length.
      if (!parse_http_headers(headers_.begin(), headers_.end(),
            content_type_, content_length_, location_))
      {
        ec = http::errc::malformed_response_headers;
        handler_(ec);
        return;
      }

      // Check the response code to see if we got the page correctly.
      if (status_code_ != http::errc::ok)
        ec = make_error_code(static_cast<http::errc::errc_t>(status_code_));

      handler_(ec);

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
    Handler handler_;
    boost::asio::ip::tcp::resolver& resolver_;
    Stream& socket_;
    const option_set& options_;
    boost::asio::streambuf& request_buffer_;
    boost::asio::streambuf& reply_buffer_;
    url url_;
    std::string& headers_;
    int status_code_;
    std::string& content_type_;
    std::size_t& content_length_;
    std::string& location_;
  };

  template <typename Handler>
  void async_open(const url& u, Handler handler)
  {
    open_coro<Handler>(handler, resolver_, socket_, options_, request_buffer_,
        reply_buffer_, u, headers_, content_type_, content_length_, location_)(
          boost::system::error_code(), 0);
  }

  boost::system::error_code close(boost::system::error_code& ec)
  {
    resolver_.cancel();
    if (!socket_.lowest_layer().close(ec))
    {
      request_buffer_.consume(request_buffer_.size());
      reply_buffer_.consume(reply_buffer_.size());
      headers_.clear();
      content_type_.clear();
      content_length_ = 0;
      location_.clear();
    }
    return ec;
  }

  bool is_open() const
  {
    return socket_.lowest_layer().is_open();
  }

  std::string content_type() const
  {
    return content_type_;
  }

  std::size_t content_length() const
  {
    return content_length_;
  }

  std::string location() const
  {
    return location_;
  }

  std::string headers() const
  {
    return headers_;
  }

  template <typename MutableBufferSequence>
  std::size_t read_some(const MutableBufferSequence& buffers,
      boost::system::error_code& ec)
  {
    // If we have any data in the reply_buffer_, return that first.
    if (reply_buffer_.size() > 0)
    {
      std::size_t bytes_transferred = 0;
      typename MutableBufferSequence::const_iterator iter = buffers.begin();
      typename MutableBufferSequence::const_iterator end = buffers.end();
      for (; iter != end && reply_buffer_.size() > 0; ++iter)
      {
        boost::asio::mutable_buffer buffer(*iter);
        size_t length = boost::asio::buffer_size(buffer);
        if (length > 0)
        {
          bytes_transferred += reply_buffer_.sgetn(
              boost::asio::buffer_cast<char*>(buffer), length);
        }
      }
      ec = boost::system::error_code();
      return bytes_transferred;
    }

    // Otherwise we forward the call to the underlying socket.
    std::size_t bytes_transferred = socket_.read_some(buffers, ec);
    if (ec == boost::asio::error::shut_down)
      ec = boost::asio::error::eof;
    return bytes_transferred;
  }

  template <typename Handler>
  class read_handler
  {
  public:
    read_handler(Handler handler)
      : handler_(handler)
    {
    }

    void operator()(boost::system::error_code ec, std::size_t bytes_transferred)
    {
      if (ec == boost::asio::error::shut_down)
        ec = boost::asio::error::eof;
      handler_(ec, bytes_transferred);
    }

    friend void* asio_handler_allocate(std::size_t size,
        read_handler<Handler>* this_handler)
    {
      using boost::asio::asio_handler_allocate;
      return asio_handler_allocate(size, &this_handler->handler_);
    }

    friend void asio_handler_deallocate(void* pointer, std::size_t size,
        read_handler<Handler>* this_handler)
    {
      using boost::asio::asio_handler_deallocate;
      asio_handler_deallocate(pointer, size, &this_handler->handler_);
    }

    template <typename Function>
    friend void asio_handler_invoke(const Function& function,
        read_handler<Handler>* this_handler)
    {
      using boost::asio::asio_handler_invoke;
      asio_handler_invoke(function, &this_handler->handler_);
    }

  private:
    Handler handler_;
  };

  template <typename MutableBufferSequence, typename Handler>
  void async_read_some(const MutableBufferSequence& buffers, Handler handler)
  {
    // If we have any data in the reply_buffer_, return that first.
    if (reply_buffer_.size() > 0)
    {
      boost::system::error_code ec;
      std::size_t bytes_transferred = read_some(buffers, ec);
      socket_.get_io_service().post(boost::asio::detail::bind_handler(
            handler, ec, bytes_transferred));
      return;
    }

    // Otherwise we forward the call to the underlying socket.
    socket_.async_read_some(buffers, read_handler<Handler>(handler));
  }

private:
  boost::asio::ip::tcp::resolver resolver_;
  Stream socket_;
  option_set& options_;
  boost::asio::streambuf request_buffer_;
  boost::asio::streambuf reply_buffer_;
  std::string headers_;
  std::string content_type_;
  std::size_t content_length_;
  std::string location_;
};

} // namespace detail
} // namespace urdl

#include "urdl/detail/abi_suffix.hpp"

#endif // URDL_DETAIL_HTTP_READ_STREAM_HPP
