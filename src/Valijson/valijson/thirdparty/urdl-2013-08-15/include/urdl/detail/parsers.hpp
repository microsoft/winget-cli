//
// parsers.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef URDL_DETAIL_PARSERS_HPP
#define URDL_DETAIL_PARSERS_HPP

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

#include "urdl/detail/abi_prefix.hpp"

namespace urdl {
namespace detail {

inline bool is_char(int c)
{
  return c >= 0 && c <= 127;
}

inline bool is_ctl(int c)
{
  return (c >= 0 && c <= 31) || c == 127;
}

inline bool is_tspecial(int c)
{
  switch (c)
  {
  case '(': case ')': case '<': case '>': case '@':
  case ',': case ';': case ':': case '\\': case '"':
  case '/': case '[': case ']': case '?': case '=':
  case '{': case '}': case ' ': case '\t':
    return true;
  default:
    return false;
  }
}

inline bool is_digit(int c)
{
  return c >= '0' && c <= '9';
}

inline bool tolower_compare(char a, char b)
{
  return std::tolower(a) == std::tolower(b);
}

inline bool headers_equal(const std::string& a, const std::string& b)
{
  if (a.length() != b.length())
    return false;
  return std::equal(a.begin(), a.end(), b.begin(), tolower_compare);
}

inline void check_header(const std::string& name, const std::string& value,
    std::string& content_type, std::size_t& content_length,
    std::string& location)
{
  if (headers_equal(name, "Content-Type"))
    content_type = value;
  else if (headers_equal(name, "Content-Length"))
    content_length = std::atoi(value.c_str());
  else if (headers_equal(name, "Location"))
    location = value;
}

template <typename Iterator>
bool parse_http_status_line(Iterator begin, Iterator end,
    int& version_major, int& version_minor, int& status)
{
  enum
  {
    http_version_h,
    http_version_t_1,
    http_version_t_2,
    http_version_p,
    http_version_slash,
    http_version_major_start,
    http_version_major,
    http_version_minor_start,
    http_version_minor,
    status_code_start,
    status_code,
    reason_phrase,
    linefeed,
    fail
  } state = http_version_h;

  Iterator iter = begin;
  std::string reason;
  while (iter != end && state != fail)
  {
    char c = *iter++;
    switch (state)
    {
    case http_version_h:
      state = (c == 'H') ? http_version_t_1 : fail;
      break;
    case http_version_t_1:
      state = (c == 'T') ? http_version_t_2 : fail;
      break;
    case http_version_t_2:
      state = (c == 'T') ? http_version_p : fail;
      break;
    case http_version_p:
      state = (c == 'P') ? http_version_slash : fail;
      break;
    case http_version_slash:
      state = (c == '/') ? http_version_major_start : fail;
      break;
    case http_version_major_start:
      if (is_digit(c))
      {
        version_major = version_major * 10 + c - '0';
        state = http_version_major;
      }
      else
        state = fail;
      break;
    case http_version_major:
      if (c == '.')
        state = http_version_minor_start;
      else if (is_digit(c))
        version_major = version_major * 10 + c - '0';
      else
        state = fail;
      break;
    case http_version_minor_start:
      if (is_digit(c))
      {
        version_minor = version_minor * 10 + c - '0';
        state = http_version_minor;
      }
      else
        state = fail;
      break;
    case http_version_minor:
      if (c == ' ')
        state = status_code_start;
      else if (is_digit(c))
        version_minor = version_minor * 10 + c - '0';
      else
        state = fail;
      break;
    case status_code_start:
      if (is_digit(c))
      {
        status = status * 10 + c - '0';
        state = status_code;
      }
      else
        state = fail;
      break;
    case status_code:
      if (c == ' ')
        state = reason_phrase;
      else if (is_digit(c))
        status = status * 10 + c - '0';
      else
        state = fail;
      break;
    case reason_phrase:
      if (c == '\r')
        state = linefeed;
      else if (is_ctl(c))
        state = fail;
      else
        reason.push_back(c);
      break;
    case linefeed:
      return (c == '\n');
    default:
      return false;
    }
  }
  return false;
}

template <typename Iterator>
bool parse_http_headers(Iterator begin, Iterator end,
    std::string& content_type, std::size_t& content_length,
    std::string& location)
{
  enum
  {
    first_header_line_start,
    header_line_start,
    header_lws,
    header_name,
    space_before_header_value,
    header_value,
    linefeed,
    final_linefeed,
    fail
  } state = first_header_line_start;

  Iterator iter = begin;
  std::string reason;
  std::string name;
  std::string value;
  while (iter != end && state != fail)
  {
    char c = *iter++;
    switch (state)
    {
    case first_header_line_start:
      if (c == '\r')
        state = final_linefeed;
      else if (!is_char(c) || is_ctl(c) || is_tspecial(c))
        state = fail;
      else
      {
        name.push_back(c);
        state = header_name;
      }
      break;
    case header_line_start:
      if (c == '\r')
      {
        check_header(name, value, content_type, content_length, location);
        name.clear();
        value.clear();
        state = final_linefeed;
      }
      else if (c == ' ' || c == '\t')
        state = header_lws;
      else if (!is_char(c) || is_ctl(c) || is_tspecial(c))
        state = fail;
      else
      {
        check_header(name, value, content_type, content_length, location);
        name.clear();
        value.clear();
        name.push_back(c);
        state = header_name;
      }
      break;
    case header_lws:
      if (c == '\r')
        state = linefeed;
      else if (c == ' ' || c == '\t')
        ; // Discard character.
      else if (is_ctl(c))
        state = fail;
      else
      {
        state = header_value;
        value.push_back(c);
      }
      break;
    case header_name:
      if (c == ':')
        state = space_before_header_value;
      else if (!is_char(c) || is_ctl(c) || is_tspecial(c))
        state = fail;
      else
        name.push_back(c);
      break;
    case space_before_header_value:
      state = (c == ' ') ? header_value : fail;
      break;
    case header_value:
      if (c == '\r')
        state = linefeed;
      else if (is_ctl(c))
        state = fail;
      else
        value.push_back(c);
      break;
    case linefeed:
      state = (c == '\n') ? header_line_start : fail;
      break;
    case final_linefeed:
      return (c == '\n');
    default:
      return false;
    }
  }
  return false;
}

} // namespace detail
} // namespace urdl

#include "urdl/detail/abi_suffix.hpp"

#endif // URDL_DETAIL_PARSERS_HPP
