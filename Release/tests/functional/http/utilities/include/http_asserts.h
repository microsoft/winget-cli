/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * http_asserts.h - Utility class to help verify assertions about http requests and responses.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#include "http_test_utilities_public.h"
#include "test_http_client.h"
#include "test_http_server.h"

namespace tests
{
namespace functional
{
namespace http
{
namespace utilities
{
template<class Char>
void trim_whitespace(std::basic_string<Char>& str)
{
    size_t index;
    // trim left whitespace
    for (index = 0; index < str.size() && isspace(str[index]); ++index)
        ;
    str.erase(0, index);
    // trim right whitespace
    for (index = str.size(); index > 0 && isspace(str[index - 1]); --index)
        ;
    str.erase(index);
}

/// <summary>
/// Helper function to do percent encoding of just the '#' character, when running under WinRT.
/// The WinRT http client implementation performs percent encoding on the '#'.
/// </summary>
TEST_UTILITY_API utility::string_t __cdecl percent_encode_pound(utility::string_t str);

/// <summary>
/// Static class containing various http request and response asserts.
/// </summary>
class http_asserts
{
public:
    /// <summary>
    /// Asserts that the specified request is equal to given arguments.
    /// </summary>
    TEST_UTILITY_API static void __cdecl assert_request_equals(web::http::http_request request,
                                                               const web::http::method& mtd,
                                                               const utility::string_t& relative_uri);

    TEST_UTILITY_API static void __cdecl assert_request_equals(
        web::http::http_request request,
        const web::http::method& mtd,
        const utility::string_t& relative_uri,
        const std::map<utility::string_t, utility::string_t>& headers);

    TEST_UTILITY_API static void __cdecl assert_request_equals(web::http::http_request request,
                                                               const web::http::method& mtd,
                                                               const utility::string_t& relative_uri,
                                                               const utility::string_t& body);

    /// <summary>
    /// Asserts that the specified response is equal to given arguments.
    /// </summary>
    TEST_UTILITY_API static void __cdecl assert_response_equals(web::http::http_response response,
                                                                const web::http::status_code& code);

    TEST_UTILITY_API static void __cdecl assert_response_equals(web::http::http_response response,
                                                                const web::http::status_code& code,
                                                                const utility::string_t& reason);

    TEST_UTILITY_API static void __cdecl assert_response_equals(
        web::http::http_response response,
        const web::http::status_code& code,
        const std::map<utility::string_t, utility::string_t>& headers);

    /// <summary>
    /// Asserts the given http_headers contains the given values.
    /// </summary>
    TEST_UTILITY_API static void __cdecl assert_http_headers_equals(const web::http::http_headers& actual,
                                                                    const web::http::http_headers& expected);

    /// <summary>
    /// Asserts the specified test_request is equal to its arguments.
    /// </summary>
    TEST_UTILITY_API static void __cdecl assert_test_request_equals(const test_request* const p_request,
                                                                    const web::http::method& mtd,
                                                                    const utility::string_t& path);

    /// <summary>
    /// Asserts the specified test_request is equal to its arguments.
    /// </summary>
    TEST_UTILITY_API static void __cdecl assert_test_request_equals(const test_request* const p_request,
                                                                    const web::http::method& mtd,
                                                                    const utility::string_t& path,
                                                                    const utility::string_t& content_type);

    /// <summary>
    /// Asserts the specified test_request is equal to its arguments.
    /// </summary>
    TEST_UTILITY_API static void __cdecl assert_test_request_contains_headers(const test_request* const p_request,
                                                                              const web::http::http_headers& headers);

    /// <summary>
    /// Asserts the specified test_request is equal to its arguments.
    /// </summary>
    TEST_UTILITY_API static void __cdecl assert_test_request_contains_headers(
        const test_request* const p_request, const std::map<utility::string_t, utility::string_t>& headers);

    /// <summary>
    /// Asserts the given HTTP request string is equal to its arguments.
    /// NOTE: this function only makes sure the specified headers exist, not that they are the only ones.
    /// </summary>
    TEST_UTILITY_API static void __cdecl assert_request_string_equals(
        const utility::string_t& request,
        const web::http::method& mtd,
        const utility::string_t& path,
        const utility::string_t& version,
        const std::map<utility::string_t, utility::string_t>& headers,
        const utility::string_t& body);

    /// <summary>
    /// Asserts the given HTTP response string is equal to its arguments.
    /// NOTE: this function only makes sure the specified headers exist, not that they are the only ones.
    /// </summary>
    TEST_UTILITY_API static void __cdecl assert_response_string_equals(
        const utility::string_t& response,
        const utility::string_t& version,
        const web::http::status_code& code,
        const utility::string_t& phrase,
        const std::map<utility::string_t, utility::string_t>& headers,
        const utility::string_t& body);

    /// <summary>
    /// Asserts the specified test_request is equal to its arguments.
    /// </summary>
    TEST_UTILITY_API static void __cdecl assert_test_request_equals(const test_request* const p_request,
                                                                    const web::http::method& mtd,
                                                                    const utility::string_t& path,
                                                                    const utility::string_t& content_type,
                                                                    const utility::string_t& body);

    /// <summary>
    /// Asserts the specified test_response is equal to its arguments.
    /// </summary>
    TEST_UTILITY_API static void __cdecl assert_test_response_equals(const test_response* const p_response,
                                                                     const web::http::status_code& code);

    TEST_UTILITY_API static void __cdecl assert_test_response_equals(
        const test_response* const p_response,
        const web::http::status_code& code,
        const std::map<utility::string_t, utility::string_t>& headers);

    TEST_UTILITY_API static void __cdecl assert_test_response_equals(const test_response* const p_response,
                                                                     const web::http::status_code& code,
                                                                     const web::http::http_headers& headers);

    TEST_UTILITY_API static void __cdecl assert_test_response_equals(test_response* p_response,
                                                                     const web::http::status_code& code,
                                                                     const utility::string_t& content_type);

    TEST_UTILITY_API static void __cdecl assert_test_response_equals(test_response* p_response,
                                                                     const web::http::status_code& code,
                                                                     const utility::string_t& content_type,
                                                                     const utility::string_t data);

private:
    http_asserts() {}
    ~http_asserts() {}
};

#if defined(_WIN32)
#if _MSC_VER >= 1900
#include <winapifamily.h>
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
// For IXMLHttpRequest with Windows 10, the error codes don't directly compare equal anymore.
// Relax verification for now.
#define HTTP_ERROR_CHECK_IMPL(__code)
#else
#define HTTP_ERROR_CHECK_IMPL(__code)                                                                                  \
    if (__code != _exc.error_code())                                                                                   \
    {                                                                                                                  \
        VERIFY_IS_TRUE(false, "Unexpected error code encountered.");                                                   \
    }
#endif
#else
// The reason we can't directly compare with the given std::errc code is because
// on Windows the STL implementation of error categories are NOT unique across
// dll boundaries, until VS2015.
#define HTTP_ERROR_CHECK_IMPL(__code)                                                                                  \
    VERIFY_ARE_EQUAL(static_cast<int>(__code), _exc.error_code().default_error_condition().value());
#endif
#else
#define HTTP_ERROR_CHECK_IMPL(__code) VERIFY_ARE_EQUAL(_exc.error_code(), __code, "Unexpected error code encountered.")
#endif

// Helper function to verify http_exception is thrown with correct error code
#define VERIFY_THROWS_HTTP_ERROR_CODE(__expression, __code)                                                            \
    UNITTEST_MULTILINE_MACRO_BEGIN                                                                                     \
    try                                                                                                                \
    {                                                                                                                  \
        __expression;                                                                                                  \
        UnitTest::CurrentTest::Results()->OnTestFailure(                                                               \
            UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__),                                        \
            "Expected exception: \"web::http::http_exception\" not thrown");                                           \
    }                                                                                                                  \
    catch (const web::http::http_exception& _exc)                                                                      \
    {                                                                                                                  \
        VERIFY_IS_TRUE(std::string(_exc.what()).size() > 0);                                                           \
        HTTP_ERROR_CHECK_IMPL(__code);                                                                                 \
    }                                                                                                                  \
    catch (const std::exception& _exc)                                                                                 \
    {                                                                                                                  \
        std::string _msg("(" #__expression ") threw exception: ");                                                     \
        _msg.append(_exc.what());                                                                                      \
        UnitTest::CurrentTest::Results()->OnTestFailure(                                                               \
            UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__), _msg.c_str());                         \
    }                                                                                                                  \
    catch (...)                                                                                                        \
    {                                                                                                                  \
        std::string _msg("(" #__expression ") threw exception: <...>");                                                \
        UnitTest::CurrentTest::Results()->OnTestFailure(                                                               \
            UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__), _msg.c_str());                         \
    }                                                                                                                  \
    UNITTEST_MULTILINE_MACRO_END

} // namespace utilities
} // namespace http
} // namespace functional
} // namespace tests
