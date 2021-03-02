//
// istreambuf.ipp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef URDL_IMPL_ISTREAMBUF_IPP
#define URDL_IMPL_ISTREAMBUF_IPP

#include <boost/array.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include "urdl/read_stream.hpp"

#include "urdl/detail/abi_prefix.hpp"

namespace urdl {

struct istreambuf::body
{
  enum { putback_max = 8 };
  enum { buffer_size = 512 };

  body()
    : read_stream_(io_service_),
      timer_(io_service_),
      open_timeout_(300 * 1000),
      read_timeout_(300 * 1000)
  {
  }

  boost::array<char, buffer_size> get_buffer_;
  boost::asio::io_service io_service_;
  boost::system::error_code error_;
  read_stream read_stream_;
  boost::asio::deadline_timer timer_;
  std::size_t open_timeout_;
  std::size_t read_timeout_;
};

namespace detail
{
  struct istreambuf_open_handler
  {
    boost::system::error_code& error_;
    boost::asio::deadline_timer& timer_;
    void operator()(boost::system::error_code ec)
    {
      error_ = ec;
      timer_.cancel();
    }
  };

  struct istreambuf_read_handler
  {
    boost::system::error_code& error_;
    std::size_t& bytes_transferred_;
    boost::asio::deadline_timer& timer_;
    void operator()(boost::system::error_code ec, std::size_t bytes_transferred)
    {
      error_ = ec;
      bytes_transferred_ = bytes_transferred;
      timer_.cancel();
    }
  };

  struct istreambuf_timeout_handler
  {
    read_stream& read_stream_;
    void operator()(boost::system::error_code ec)
    {
      if (ec != boost::asio::error::operation_aborted)
        read_stream_.close(ec);
    }
  };
} // namespace detail

istreambuf::istreambuf()
  : body_(new body)
{
  init_buffers();
}

istreambuf::~istreambuf()
{
  try
  {
    delete body_;
  }
  catch (std::exception&)
  {
    // Swallow the exception.
  }
}

void istreambuf::set_options(const option_set& options)
{
  body_->read_stream_.set_options(options);
}

option_set istreambuf::get_options() const
{
  return body_->read_stream_.get_options();
}

istreambuf* istreambuf::open(const url& u)
{
  if (is_open())
    return 0;

  init_buffers();
  body_->read_stream_.close(body_->error_);

  detail::istreambuf_open_handler oh = { body_->error_, body_->timer_ };
  body_->read_stream_.async_open(u, oh);

  detail::istreambuf_timeout_handler th = { body_->read_stream_ };
  body_->timer_.expires_from_now(
      boost::posix_time::milliseconds(body_->open_timeout_));
  body_->timer_.async_wait(th);

  body_->io_service_.reset();
  body_->io_service_.run();

  if (!body_->read_stream_.is_open())
    body_->error_ = make_error_code(boost::system::errc::timed_out);

  return !body_->error_ ? this : 0;
}

bool istreambuf::is_open() const
{
  return body_->read_stream_.is_open();
}

istreambuf* istreambuf::close()
{
  if (!is_open())
    return 0;

  body_->read_stream_.close(body_->error_);
  if (!body_->error_)
    init_buffers();
  return !body_->error_ ? this : 0;
}

const boost::system::error_code& istreambuf::puberror() const
{
  return error();
}

std::size_t istreambuf::open_timeout() const
{
  return body_->open_timeout_;
}

void istreambuf::open_timeout(std::size_t milliseconds)
{
  body_->open_timeout_ = milliseconds;
}

std::size_t istreambuf::read_timeout() const
{
  return body_->read_timeout_;
}

void istreambuf::read_timeout(std::size_t milliseconds)
{
  body_->read_timeout_ = milliseconds;
}

std::string istreambuf::content_type() const
{
  return body_->read_stream_.content_type();
}

std::size_t istreambuf::content_length() const
{
  return body_->read_stream_.content_length();
}

std::string istreambuf::headers() const
{
  return body_->read_stream_.headers();
}

std::streambuf::int_type istreambuf::underflow()
{
  if (gptr() == egptr())
  {
    std::size_t bytes_transferred = 0;
    detail::istreambuf_read_handler rh
      = { body_->error_, bytes_transferred, body_->timer_ };
    body_->read_stream_.async_read_some(boost::asio::buffer(
          boost::asio::buffer(body_->get_buffer_) + body::putback_max), rh);

    detail::istreambuf_timeout_handler th = { body_->read_stream_ };
    body_->timer_.expires_from_now(
        boost::posix_time::milliseconds(body_->read_timeout_));
    body_->timer_.async_wait(th);

    body_->io_service_.reset();
    body_->io_service_.run();

    if (!body_->read_stream_.is_open())
      body_->error_ = make_error_code(boost::system::errc::timed_out);

    if (body_->error_)
    {
      if (body_->error_ == boost::asio::error::eof)
      {
        body_->error_ = boost::system::error_code();
        return traits_type::eof();
      }
      boost::throw_exception(boost::system::system_error(body_->error_));
    }

    setg(body_->get_buffer_.begin(),
        body_->get_buffer_.begin() + body::putback_max,
        body_->get_buffer_.begin() + body::putback_max + bytes_transferred);
    return traits_type::to_int_type(*gptr());
  }
  else
  {
    return traits_type::eof();
  }
}

const boost::system::error_code& istreambuf::error() const
{
  return body_->error_;
}

void istreambuf::init_buffers()
{
  setg(body_->get_buffer_.begin(),
      body_->get_buffer_.begin() + body::putback_max,
      body_->get_buffer_.begin() + body::putback_max);
}

} // namespace urdl

#include "urdl/detail/abi_suffix.hpp"

#endif // URDL_IMPL_ISTREAMBUF_IPP
