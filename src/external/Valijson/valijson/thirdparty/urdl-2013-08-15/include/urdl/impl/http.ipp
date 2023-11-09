//
// http.ipp
// ~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef URDL_HTTP_IPP
#define URDL_HTTP_IPP

#include <boost/system/error_code.hpp>

#include "urdl/detail/abi_prefix.hpp"

namespace urdl {
namespace http {
namespace detail {

class error_category_impl
  : public boost::system::error_category
{
  virtual const char* name() const URDL_ERROR_CATEGORY_NOEXCEPT
  {
    return "HTTP";
  }

  virtual std::string message(int e) const
  {
    switch (e)
    {
    case http::errc::malformed_status_line:
      return "Malformed status line";
    case http::errc::malformed_response_headers:
      return "Malformed response headers";
    case http::errc::continue_request:
      return "Continue";
    case http::errc::switching_protocols:
      return "Switching protocols";
    case http::errc::ok:
      return "OK";
    case http::errc::created:
      return "Created";
    case http::errc::accepted:
      return "Accepted";
    case http::errc::non_authoritative_information:
      return "Non-authoritative information";
    case http::errc::no_content:
      return "No content";
    case http::errc::reset_content:
      return "Reset content";
    case http::errc::partial_content:
      return "Partial content";
    case http::errc::multiple_choices:
      return "Multiple choices";
    case http::errc::moved_permanently:
      return "Moved permanently";
    case http::errc::found:
      return "Found";
    case http::errc::see_other:
      return "See other";
    case http::errc::not_modified:
      return "Not modified";
    case http::errc::use_proxy:
      return "Use proxy";
    case http::errc::temporary_redirect:
      return "Temporary redirect";
    case http::errc::bad_request:
      return "Bad request";
    case http::errc::unauthorized:
      return "Unauthorized";
    case http::errc::payment_required:
      return "Payment required";
    case http::errc::forbidden:
      return "Forbidden";
    case http::errc::not_found:
      return "Not found";
    case http::errc::method_not_allowed:
      return "Method not allowed";
    case http::errc::not_acceptable:
      return "Not acceptable";
    case http::errc::proxy_authentication_required:
      return "Proxy authentication required";
    case http::errc::request_timeout:
      return "Request time-out";
    case http::errc::conflict:
      return "Conflict";
    case http::errc::gone:
      return "Gone";
    case http::errc::length_required:
      return "Length required";
    case http::errc::precondition_failed:
      return "Precondition failed";
    case http::errc::request_entity_too_large:
      return "Request entity too large";
    case http::errc::request_uri_too_large:
      return "Request URI too large";
    case http::errc::unsupported_media_type:
      return "Unsupported media type";
    case http::errc::requested_range_not_satisfiable:
      return "Requested range not satisfiable";
    case http::errc::expectation_failed:
      return "Expectation failed";
    case http::errc::internal_server_error:
      return "Internal server error";
    case http::errc::not_implemented:
      return "Not implemented";
    case http::errc::bad_gateway:
      return "Bad gateway";
    case http::errc::service_unavailable:
      return "Service unavailable";
    case http::errc::gateway_timeout:
      return "Gateway time-out";
    case http::errc::version_not_supported:
      return "HTTP version not supported";
    default:
      return "Unknown HTTP error";
    }
  }

  virtual boost::system::error_condition default_error_condition(
      int e) const URDL_ERROR_CATEGORY_NOEXCEPT
  {
    switch (e)
    {
    case http::errc::unauthorized:
    case http::errc::forbidden:
      return boost::system::errc::permission_denied;
    case http::errc::not_found:
      return boost::system::errc::no_such_file_or_directory;
    default:
      return boost::system::error_condition(e, *this);
    }
  }
};

} // namespace detail

const boost::system::error_category& error_category()
{
  static detail::error_category_impl instance;
  return instance;
}

namespace detail {

static const boost::system::error_category& category_instance
  = error_category();

} // namespace detail
} // namespace http
} // namespace urdl

#include "urdl/detail/abi_suffix.hpp"

#endif // URDL_HTTP_IPP
