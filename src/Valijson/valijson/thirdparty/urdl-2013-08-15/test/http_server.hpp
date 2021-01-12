//
// http_server.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <boost/asio/buffer.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

// Helper class to test HTTP client functionality.
class http_server
{
public:
  typedef boost::asio::ip::tcp tcp;

  http_server()
    : acceptor_(io_service_, tcp::endpoint(
          boost::asio::ip::address_v4::loopback(), 0)),
      socket_(io_service_),
      response_delay_(0),
      content_delay_(0),
      success_(false)
  {
  }

  unsigned short port() const
  {
    return acceptor_.local_endpoint().port();
  }

  void start(const std::string& expected_request,
      std::size_t response_delay, const std::string& response,
      std::size_t content_delay, const std::string& content)
  {
    success_ = false;
    expected_request_ = expected_request;
    response_delay_ = response_delay;
    response_ = response;
    content_delay_ = content_delay;
    content_ = content;
    thread_.reset(new boost::thread(boost::bind(&http_server::worker, this)));
  }

  bool stop()
  {
    thread_->join();
    thread_.reset();
    return success_;
  }

private:
  void worker()
  {
    try
    {
      acceptor_.accept(socket_);

      // Wait for request.
      boost::asio::streambuf buffer;
      std::size_t size = boost::asio::read_until(socket_, buffer, "\r\n\r\n");
      std::string request(size, 0);
      buffer.sgetn(&request[0], size);
      success_ = (request == expected_request_);

      // Introduce a delay before sending the response.
      boost::asio::deadline_timer timer(io_service_);
      timer.expires_from_now(boost::posix_time::milliseconds(response_delay_));
      timer.wait();

      // Send response headers.
      boost::system::error_code ec;
      boost::asio::write(socket_, boost::asio::buffer(response_));

      // Introduce a delay before sending the content.
      timer.expires_from_now(boost::posix_time::milliseconds(content_delay_));
      timer.wait();

      // Now we can write the content.
      boost::asio::write(socket_, boost::asio::buffer(content_));

      // We're done. Shut down the connection.
      socket_.shutdown(tcp::socket::shutdown_both, ec);
      socket_.close(ec);
    }
    catch (std::exception&)
    {
    }
  }

  boost::asio::io_service io_service_;
  tcp::acceptor acceptor_;
  tcp::socket socket_;
  std::string expected_request_;
  std::size_t response_delay_;
  std::string response_;
  std::size_t content_delay_;
  std::string content_;
  boost::scoped_ptr<boost::thread> thread_;
  bool success_;
};

#endif // HTTP_SERVER_HPP
