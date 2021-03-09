/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Contains utility functions for helping to verify server certificates in OS X/iOS and Android.
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#if defined(_WIN32)
#include <Wincrypt.h>

namespace web
{
namespace http
{
namespace client
{
namespace details
{
struct winhttp_cert_context
{
    PCCERT_CONTEXT raw;
    winhttp_cert_context() CPPREST_NOEXCEPT : raw(nullptr) {}
    winhttp_cert_context(const winhttp_cert_context&) = delete;
    winhttp_cert_context& operator=(const winhttp_cert_context&) = delete;
    ~winhttp_cert_context()
    {
        // https://docs.microsoft.com/en-us/windows/desktop/api/wincrypt/nf-wincrypt-certfreecertificatecontext
        // "The function always returns nonzero."
        if (raw)
        {
            (void)CertFreeCertificateContext(raw);
        }
    }
};

struct winhttp_cert_chain_context
{
    PCCERT_CHAIN_CONTEXT raw;
    winhttp_cert_chain_context() CPPREST_NOEXCEPT : raw(nullptr) {}
    winhttp_cert_chain_context(const winhttp_cert_chain_context&) = delete;
    winhttp_cert_chain_context& operator=(const winhttp_cert_chain_context&) = delete;
    ~winhttp_cert_chain_context()
    {
        if (raw)
        {
            CertFreeCertificateChain(raw);
        }
    }
};
} // namespace details
} // namespace client
} // namespace http
} // namespace web
#endif // _WIN32

#if defined(__APPLE__) || (defined(ANDROID) || defined(__ANDROID__)) ||                                                \
    (defined(_WIN32) && defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)) ||                                                    \
    (defined(_WIN32) && !defined(__cplusplus_winrt) && !defined(_M_ARM) && !defined(CPPREST_EXCLUDE_WEBSOCKETS))
#define CPPREST_PLATFORM_ASIO_CERT_VERIFICATION_AVAILABLE
#endif

#ifdef CPPREST_PLATFORM_ASIO_CERT_VERIFICATION_AVAILABLE
#include <string>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4005)
#endif
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif
#include <boost/asio/ssl.hpp>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace web
{
namespace http
{
namespace client
{
namespace details
{
/// <summary>
/// Using platform specific APIs verifies server certificate.
/// Currently implemented to work on Windows, iOS, Android, and OS X.
/// </summary>
/// <param name="verifyCtx">Boost.ASIO context to get certificate chain from.</param>
/// <param name="hostName">Host name from the URI.</param>
/// <returns>True if verification passed and server can be trusted, false otherwise.</returns>
bool verify_cert_chain_platform_specific(boost::asio::ssl::verify_context& verifyCtx, const std::string& hostName);
} // namespace details
} // namespace client
} // namespace http
} // namespace web

#endif // CPPREST_PLATFORM_ASIO_CERT_VERIFICATION_AVAILABLE
