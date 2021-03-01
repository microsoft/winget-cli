//
// handshake.hpp
// ~~~~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef URDL_DETAIL_HANDSHAKE_HPP
#define URDL_DETAIL_HANDSHAKE_HPP

#include <cstring>
#include <cctype>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include "urdl/detail/coroutine.hpp"

#if !defined(URDL_DISABLE_SSL)
# include <boost/asio/ssl.hpp>
# include <openssl/x509v3.h>
#endif // !defined(URDL_DISABLE_SSL)

#include "urdl/detail/abi_prefix.hpp"

namespace urdl {
namespace detail {

inline boost::system::error_code handshake(
    boost::asio::ip::tcp::socket& /*socket*/,
    const std::string& /*host*/, boost::system::error_code& ec)
{
  ec = boost::system::error_code();
  return ec;
}

template <typename Handler>
void async_handshake(boost::asio::ip::tcp::socket& socket,
    const std::string& /*host*/, Handler handler)
{
  boost::system::error_code ec;
  socket.get_io_service().post(boost::asio::detail::bind_handler(handler, ec));
}

#if !defined(URDL_DISABLE_SSL)
inline bool match_pattern(const char* pattern,
    std::size_t pattern_length, const char* host)
{
  const char* p = pattern;
  const char* p_end = p + pattern_length;
  const char* h = host;

  while (p != p_end && *h)
  {
    if (*p == '*')
    {
      ++p;
      while (*h && *h != '.')
        if (match_pattern(p, p_end - p, h++))
          return true;
    }
    else if (std::tolower(*p) == std::tolower(*h))
    {
      ++p;
      ++h;
    }
    else
    {
      return false;
    }
  }

  return p == p_end && !*h;
}

inline bool certificate_matches_host(X509* cert, const std::string& host)
{
  // Try converting host name to an address. If it is an address then we need
  // to look for an IP address in the certificate rather than a host name.
  boost::system::error_code ec;
  boost::asio::ip::address address
    = boost::asio::ip::address::from_string(host, ec);
  bool is_address = !ec;

  // Go through the alternate names in the certificate looking for matching DNS
  // or IP address entries.
  GENERAL_NAMES* gens = static_cast<GENERAL_NAMES*>(
      X509_get_ext_d2i(cert, NID_subject_alt_name, 0, 0));
  for (int i = 0; i < sk_GENERAL_NAME_num(gens); ++i)
  {
    GENERAL_NAME* gen = sk_GENERAL_NAME_value(gens, i);
    if (gen->type == GEN_DNS && !is_address)
    {
      ASN1_IA5STRING* domain = gen->d.dNSName;
      if (domain->type == V_ASN1_IA5STRING
          && domain->data && domain->length)
      {
        const char* pattern = reinterpret_cast<const char*>(domain->data);
        std::size_t pattern_length = domain->length;
        if (match_pattern(pattern, pattern_length, host.c_str()))
        {
          GENERAL_NAMES_free(gens);
          return true;
        }
      }
    }
    else if (gen->type == GEN_IPADD && is_address)
    {
      ASN1_OCTET_STRING* ip_address = gen->d.iPAddress;
      if (ip_address->type == V_ASN1_OCTET_STRING && ip_address->data)
      {
        if (address.is_v4() && ip_address->length == 4)
        {
          boost::asio::ip::address_v4::bytes_type address_bytes
            = address.to_v4().to_bytes();
          if (std::memcmp(address_bytes.data(), ip_address->data, 4) == 0)
          {
            GENERAL_NAMES_free(gens);
            return true;
          }
        }
        else if (address.is_v6() && ip_address->length == 16)
        {
          boost::asio::ip::address_v6::bytes_type address_bytes
            = address.to_v6().to_bytes();
          if (std::memcmp(address_bytes.data(), ip_address->data, 16) == 0)
          {
            GENERAL_NAMES_free(gens);
            return true;
          }
        }
      }
    }
  }
  GENERAL_NAMES_free(gens);

  // No match in the alternate names, so try the common names. We should only
  // use the "most specific" common name, which is the last one in the list.
  X509_NAME* name = X509_get_subject_name(cert);
  int i = -1;
  ASN1_STRING* common_name = 0;
  while ((i = X509_NAME_get_index_by_NID(name, NID_commonName, i)) >= 0)
  {
    X509_NAME_ENTRY* name_entry = X509_NAME_get_entry(name, i);
    common_name = X509_NAME_ENTRY_get_data(name_entry);
  }
  if (common_name && common_name->data && common_name->length)
  {
    const char* pattern = reinterpret_cast<const char*>(common_name->data);
    std::size_t pattern_length = common_name->length;
    if (match_pattern(pattern, pattern_length, host.c_str()))
      return true;
  }

  return false;
}

inline boost::system::error_code handshake(
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& socket,
    const std::string& host, boost::system::error_code& ec)
{
  // Perform SSL handshake.
  socket.handshake(boost::asio::ssl::stream_base::client, ec);
  if (ec)
    return ec;

  // Verify the certificate returned by the host.
  if (X509* cert = SSL_get_peer_certificate(socket.impl()->ssl))
  {
    if (SSL_get_verify_result(socket.impl()->ssl) == X509_V_OK)
    {
      if (certificate_matches_host(cert, host))
        ec = boost::system::error_code();
      else
        ec = make_error_code(boost::system::errc::permission_denied);
    }
    else
      ec = make_error_code(boost::system::errc::permission_denied);
    X509_free(cert);
  }
  else
    ec = make_error_code(boost::system::errc::permission_denied);

  return ec;
}

template <typename Handler>
class handshake_coro : coroutine
{
public:
  handshake_coro(Handler handler,
      boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& socket,
      const std::string& host)
    : handler_(handler),
      socket_(socket),
      host_(host)
  {
  }

  void operator()(boost::system::error_code ec)
  {
    URDL_CORO_BEGIN;

    // Perform SSL handshake.
    URDL_CORO_YIELD(socket_.async_handshake(
          boost::asio::ssl::stream_base::client, *this));
    if (ec)
    {
      handler_(ec);
      return;
    }

    // Verify the certificate returned by the host.
    if (X509* cert = SSL_get_peer_certificate(socket_.impl()->ssl))
    {
      if (SSL_get_verify_result(socket_.impl()->ssl) == X509_V_OK)
      {
        if (certificate_matches_host(cert, host_))
          ec = boost::system::error_code();
        else
          ec = make_error_code(boost::system::errc::permission_denied);
      }
      else
        ec = make_error_code(boost::system::errc::permission_denied);
      X509_free(cert);
    }
    else
      ec = make_error_code(boost::system::errc::permission_denied);

    handler_(ec);

    URDL_CORO_END;
  }

  friend void* asio_handler_allocate(std::size_t size,
      handshake_coro<Handler>* this_handler)
  {
    using boost::asio::asio_handler_allocate;
    return asio_handler_allocate(size, &this_handler->handler_);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size,
      handshake_coro<Handler>* this_handler)
  {
    using boost::asio::asio_handler_deallocate;
    asio_handler_deallocate(pointer, size, &this_handler->handler_);
  }

  template <typename Function>
  friend void asio_handler_invoke(const Function& function,
      handshake_coro<Handler>* this_handler)
  {
    using boost::asio::asio_handler_invoke;
    asio_handler_invoke(function, &this_handler->handler_);
  }

private:
  Handler handler_;
  boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& socket_;
  std::string host_;
};

template <typename Handler>
void async_handshake(
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& socket,
    const std::string& host, Handler handler)
{
  handshake_coro<Handler>(handler, socket, host)(boost::system::error_code());
}
#endif // !defined(URDL_DISABLE_SSL)

} // namespace detail
} // namespace urdl

#include "urdl/detail/abi_suffix.hpp"

#endif // URDL_DETAIL_HANDSHAKE_HPP
