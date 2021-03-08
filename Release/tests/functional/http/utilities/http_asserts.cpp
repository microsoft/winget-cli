/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * http_asserts.cpp - Utility class to help verify assertions about http requests and responses.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#include "stdafx.h"

using namespace web;
using namespace utility;
using namespace utility::conversions;

namespace tests
{
namespace functional
{
namespace http
{
namespace utilities
{
utility::string_t percent_encode_pound(utility::string_t str)
{
    size_t index;
    while ((index = str.find_first_of(U("#"))) != str.npos)
    {
        str.insert(index, U("%23"));
        str.erase(index + 3, 1);
    }
    return str;
}

// Helper function to verify all given headers are present.
template<typename T1, typename T2>
static void verify_headers(const T1& expected, const T2& actual)
{
    for (auto iter = expected.begin(); iter != expected.end(); ++iter)
    {
        VERIFY_ARE_EQUAL(iter->second, actual.find(iter->first)->second);
    }
}

void http_asserts::assert_request_equals(::http::http_request request,
                                         const ::http::method& mtd,
                                         const utility::string_t& relative_path)
{
    VERIFY_ARE_EQUAL(mtd, request.method());
    if (relative_path == U(""))
    {
        VERIFY_ARE_EQUAL(U("/"), request.relative_uri().to_string());
    }
    else
    {
        VERIFY_ARE_EQUAL(relative_path, request.relative_uri().to_string());
    }
}

void http_asserts::assert_request_equals(::http::http_request request,
                                         const ::http::method& mtd,
                                         const utility::string_t& relative_uri,
                                         const std::map<utility::string_t, utility::string_t>& headers)
{
    assert_request_equals(request, mtd, relative_uri);
    verify_headers(headers, request.headers());
}

void http_asserts::assert_request_equals(::http::http_request request,
                                         const ::http::method& mtd,
                                         const utility::string_t& relative_path,
                                         const utility::string_t& body)
{
    assert_request_equals(request, mtd, relative_path);
    auto request_data = request.extract_string().get();
    VERIFY_ARE_EQUAL(body, request_data);
}

void http_asserts::assert_response_equals(::http::http_response response, const ::http::status_code& code)
{
    VERIFY_ARE_EQUAL(response.status_code(), code);
}

void http_asserts::assert_response_equals(::http::http_response response,
                                          const ::http::status_code& code,
                                          const utility::string_t& reason)
{
    VERIFY_ARE_EQUAL(code, response.status_code());
    VERIFY_ARE_EQUAL(reason, response.reason_phrase());
}

void http_asserts::assert_response_equals(::http::http_response response,
                                          const ::http::status_code& code,
                                          const std::map<utility::string_t, utility::string_t>& headers)
{
    VERIFY_ARE_EQUAL(code, response.status_code());
    verify_headers(headers, response.headers());
}

void http_asserts::assert_http_headers_equals(const ::http::http_headers& actual, const ::http::http_headers& expected)
{
    verify_headers(actual, expected);
}

void http_asserts::assert_test_request_equals(const test_request* const p_request,
                                              const ::http::method& mtd,
                                              const utility::string_t& path)
{
    VERIFY_ARE_EQUAL(mtd, p_request->m_method);
    VERIFY_ARE_EQUAL(path, p_request->m_path);
}

void http_asserts::assert_test_request_equals(const test_request* const p_request,
                                              const ::http::method& mtd,
                                              const utility::string_t& path,
                                              const utility::string_t& content_type)
{
    VERIFY_ARE_EQUAL(mtd, p_request->m_method);
    VERIFY_ARE_EQUAL(path, p_request->m_path);

    // verify that content-type key exists in the header and the value matches the one provided
    auto iter = p_request->m_headers.find(U("Content-Type"));
    if (content_type.empty())
    {
        VERIFY_ARE_EQUAL(iter, p_request->m_headers.end());
    }
    else
    {
        VERIFY_IS_TRUE(iter != p_request->m_headers.end());
        VERIFY_ARE_EQUAL(iter->second.find(content_type), 0);
    }
}

void http_asserts::assert_test_request_contains_headers(const test_request* const p_request,
                                                        const ::http::http_headers& headers)
{
    verify_headers(headers, p_request->m_headers);
}

void http_asserts::assert_test_request_contains_headers(const test_request* const p_request,
                                                        const std::map<utility::string_t, utility::string_t>& headers)
{
    verify_headers(headers, p_request->m_headers);
}

// Helper function to parse HTTP headers from a stringstream.
static std::map<utility::string_t, utility::string_t> parse_headers(utility::istringstream_t& ss)
{
    // Keep parsing until CRLF is encountered.
    std::map<utility::string_t, utility::string_t> headers;
    utility::string_t header_line;
    while (getline(ss, header_line).good())
    {
        const size_t colon_index = header_line.find(U(":"));
        const utility::string_t header_name = header_line.substr(0, colon_index);
        utility::string_t header_value = header_line.substr(colon_index + 1);
        tests::functional::http::utilities::trim_whitespace(header_value);
        headers[header_name] = header_value;

        char c1 = (char)ss.get(), c2 = (char)ss.get();
        if (c1 == '\r' && c2 == '\n')
        {
            break;
        }
        ss.unget();
        ss.unget();
    }
    return headers;
}

void http_asserts::assert_request_string_equals(const utility::string_t& request,
                                                const ::http::method& mtd,
                                                const utility::string_t& path,
                                                const utility::string_t& version,
                                                const std::map<utility::string_t, utility::string_t>& headers,
                                                const utility::string_t& body)
{
    utility::istringstream_t ss(request);

    // Parse request line.
    utility::string_t actual_method, actual_path, actual_version;
    ss >> actual_method >> actual_path >> actual_version;

    // Parse headers.
    utility::string_t temp;
    getline(ss, temp);
    std::map<utility::string_t, utility::string_t> actual_headers = parse_headers(ss);

    // Parse in any message body
    utility::string_t actual_body = ss.str().substr((size_t)ss.tellg());

    VERIFY_ARE_EQUAL(mtd, actual_method);
    VERIFY_ARE_EQUAL(path, actual_path);
    VERIFY_ARE_EQUAL(version, actual_version);
    verify_headers(headers, actual_headers);
    VERIFY_ARE_EQUAL(body, actual_body);
}

void http_asserts::assert_response_string_equals(const utility::string_t& response,
                                                 const utility::string_t& version,
                                                 const ::http::status_code& code,
                                                 const utility::string_t& phrase,
                                                 const std::map<utility::string_t, utility::string_t>& headers,
                                                 const utility::string_t& body)
{
    utility::istringstream_t ss(response);

    // Parse response line.
    utility::string_t actual_version, actual_phrase;
    ::http::status_code actual_code;
    ss >> actual_version >> actual_code >> actual_phrase;

    // Parse headers.
    utility::string_t temp;
    getline(ss, temp);
    std::map<utility::string_t, utility::string_t> actual_headers = parse_headers(ss);

    // Prase in any message body.
    utility::string_t actual_body = ss.str().substr((size_t)ss.tellg());

    VERIFY_ARE_EQUAL(version, actual_version);
    VERIFY_ARE_EQUAL(code, actual_code);
    VERIFY_ARE_EQUAL(phrase, actual_phrase);
    verify_headers(headers, actual_headers);
    VERIFY_ARE_EQUAL(body, actual_body);
}

void http_asserts::assert_test_request_equals(const test_request* const p_request,
                                              const ::http::method& mtd,
                                              const utility::string_t& path,
                                              const utility::string_t& content_type,
                                              const utility::string_t& body)
{
    assert_test_request_equals(p_request, mtd, path, content_type);
    // Textual response is always sent as UTF-8, hence the converison to string_t
    std::string s((char*)&p_request->m_body[0], p_request->m_body.size());
    utility::string_t extracted_body = to_string_t(s);

    VERIFY_ARE_EQUAL(body, extracted_body);
}

void http_asserts::assert_test_response_equals(const test_response* const p_response, const ::http::status_code& code)
{
    VERIFY_ARE_EQUAL(code, p_response->m_status_code);
}

void http_asserts::assert_test_response_equals(const test_response* const p_response,
                                               const ::http::status_code& code,
                                               const std::map<utility::string_t, utility::string_t>& headers)
{
    VERIFY_ARE_EQUAL(code, p_response->m_status_code);
    verify_headers(headers, p_response->m_headers);
}

void http_asserts::assert_test_response_equals(const test_response* const p_response,
                                               const ::http::status_code& code,
                                               const ::http::http_headers& headers)
{
    VERIFY_ARE_EQUAL(code, p_response->m_status_code);
    verify_headers(headers, p_response->m_headers);
}

void http_asserts::assert_test_response_equals(test_response* p_response,
                                               const ::http::status_code& code,
                                               const utility::string_t& content_type)
{
    VERIFY_ARE_EQUAL(code, p_response->m_status_code);
    utility::string_t found_content;
    p_response->match_header(U("Content-Type"), found_content);
    VERIFY_ARE_EQUAL(content_type, found_content);
}

void http_asserts::assert_test_response_equals(test_response* p_response,
                                               const ::http::status_code& code,
                                               const utility::string_t& content_type,
                                               const utility::string_t data)
{
    VERIFY_ARE_EQUAL(code, p_response->m_status_code);
    utility::string_t found_content;
    p_response->match_header(U("Content-Type"), found_content);
    VERIFY_ARE_EQUAL(found_content.find(content_type), 0);

    // Beware: what kind of string this is? <-- stringhack until we tighten up wide/narrow string business
    utility::string_t extracted_body;
    if (p_response->m_data.size() == 0)
    {
        extracted_body = U("");
    }
    else
    {
        auto actualRawData = (char*)&p_response->m_data[0];
        if (p_response->m_data.size() > 1 && *(actualRawData + 1) == '\0')
        {
            // We have more than one byte of data, but it's null-terminated at byte 1.
            // Therefore, this is a wide string
            extracted_body.assign((utility::char_t*)actualRawData, p_response->m_data.size() / sizeof(utility::char_t));
        }
        else
        {
            std::string s(actualRawData, p_response->m_data.size());
            extracted_body = to_string_t(s);
        }
    }

    VERIFY_ARE_EQUAL(data, extracted_body);
}

} // namespace utilities
} // namespace http
} // namespace functional
} // namespace tests
