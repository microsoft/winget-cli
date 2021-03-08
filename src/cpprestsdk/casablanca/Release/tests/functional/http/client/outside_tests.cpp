/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Tests cases for using http_clients to outside websites.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"
#if defined(_MSC_VER) && !defined(__cplusplus_winrt)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <winhttp.h>
#pragma comment(lib, "winhttp")
#endif
#include "cpprest/details/http_helpers.h"
#include "cpprest/rawptrstream.h"
#include "os_utilities.h"
#include <stdexcept>

using namespace web;
using namespace utility;
using namespace concurrency;
using namespace web::http;
using namespace web::http::client;

using namespace tests::common::utilities;
using namespace tests::functional::http::utilities;

namespace tests
{
namespace functional
{
namespace http
{
namespace client
{
SUITE(outside_tests)
{
    TEST_FIXTURE(uri_address, outside_cnn_dot_com)
    {
        handle_timeout([] {
            // http://www.cnn.com redirects users from countries outside of the US to the "http://edition.cnn.com/" drop
            // location
            http_client client(U("http://edition.cnn.com"));

            // CNN's main page doesn't use chunked transfer encoding.
            http_response response = client.request(methods::GET).get();
            VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
            response.content_ready().wait();

            // CNN's other pages do use chunked transfer encoding.
            response = client.request(methods::GET, U("us")).get();
            VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
            response.content_ready().wait();
        });
    }

    TEST_FIXTURE(uri_address, outside_wikipedia_compressed_http_response)
    {
        if (web::http::compression::builtin::supported() == false)
        {
            // On platforms which do not support compressed http, nothing to check.
            return;
        }
        http_client_config config;
        config.set_request_compressed_response(true);

        http_client client(U("https://en.wikipedia.org/wiki/HTTP_compression"), config);
        http_request httpRequest(methods::GET);

        http_response response = client.request(httpRequest).get();
        VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
        response.content_ready().wait();

        auto s = response.extract_utf8string().get();
        VERIFY_IS_FALSE(s.empty());

        utility::string_t encoding;
        VERIFY_IS_TRUE(response.headers().match(web::http::header_names::content_encoding, encoding));

        VERIFY_ARE_EQUAL(encoding, U("gzip"));
    }

    TEST_FIXTURE(uri_address, outside_google_dot_com)
    {
        // Use code.google.com instead of www.google.com, which redirects
        http_client client(U("http://code.google.com"));
        http_request request(methods::GET);
        for (int i = 0; i < 2; ++i)
        {
            http_response response = client.request(request).get();
            VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
        }
    }

    TEST_FIXTURE(uri_address, multiple_https_requests)
    {
        handle_timeout([&] {
            // Use code.google.com instead of www.google.com, which redirects
            http_client client(U("https://code.google.com"));

            http_response response;
            for (int i = 0; i < 5; ++i)
            {
                response = client.request(methods::GET).get();
                VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
                response.content_ready().wait();
            }
        });
    }

#if (defined(_MSC_VER) && (_MSC_VER >= 1900)) && !CPPREST_FORCE_PPLX
    TEST_FIXTURE(uri_address, multiple_https_requests_sync_scheduler)
    {
        struct sync_scheduler : public scheduler_interface
        {
        public:
            virtual void schedule(TaskProc_t function, PVOID context) override { function(context); }
        };

        // Save the current ambient scheduler
        const auto scheduler = get_cpprestsdk_ambient_scheduler();

        // Change the ambient scheduler to one that schedules synchronously
        static std::shared_ptr<scheduler_interface> syncScheduler = std::make_shared<sync_scheduler>();
        set_cpprestsdk_ambient_scheduler(syncScheduler);

        handle_timeout([&] {
            // Use code.google.com instead of www.google.com, which redirects
            http_client client(U("https://code.google.com"));

            http_response response;
            for (int i = 0; i < 5; ++i)
            {
                response = client.request(methods::GET).get();
                VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
                response.content_ready().wait();
            }
        });

        // Revert to the original scheduler
        set_cpprestsdk_ambient_scheduler(scheduler);
    }
#endif

    TEST_FIXTURE(uri_address, reading_google_stream)
    {
        handle_timeout([&] {
            // Use code.google.com instead of www.google.com, which redirects
            http_client simpleclient(U("http://code.google.com"));
            utility::string_t path = m_uri.query();
            http_response response = simpleclient.request(::http::methods::GET).get();

            uint8_t chars[71];
            memset(chars, 0, sizeof(chars));

            streams::rawptr_buffer<uint8_t> temp(chars, sizeof(chars));

            VERIFY_ARE_EQUAL(response.body().read(temp, 70).get(), 70);
            // Uncomment the following line to output the chars.
            // std::cout << chars << '\n';
            VERIFY_ARE_EQUAL(strcmp((const char*)chars,
                                    "<html>\n  <head>\n    <meta name=\"google-site-verification\" content=\"4zc"),
                             0);
        });
    }

    TEST_FIXTURE(uri_address, no_transfer_encoding_content_length)
    {
        handle_timeout([] {
            http_client client(U("http://ws.audioscrobbler.com/2.0/") U(
                "?method=artist.gettoptracks&artist=cher&api_key=6fcd59047568e89b1615975081258990&format=json"));

            client.request(methods::GET)
                .then([](http_response response) {
                    VERIFY_ARE_EQUAL(response.status_code(), status_codes::OK);
                    VERIFY_IS_FALSE(response.headers().has(header_names::content_length) &&
                                    response.headers().has(header_names::transfer_encoding));
                    return response.extract_string();
                })
                .then([](string_t result) {
                    // Verify that the body size isn't empty.
                    VERIFY_IS_TRUE(result.size() > 0);
                })
                .wait();
        });
    }

    // Note additional sites for testing can be found at:
    // https://badssl.com/
    // https://www.ssllabs.com/ssltest/
    // http://www.internetsociety.org/deploy360/resources/dane-test-sites/
    // https://onlinessl.netlock.hu/#
    static void test_failed_ssl_cert(const uri& base_uri)
    {
        handle_timeout([&base_uri] {
            http_client client(base_uri);
            auto requestTask = client.request(methods::GET);
            VERIFY_THROWS(requestTask.get(), http_exception);
        });
    }

#if !defined(__cplusplus_winrt)
    static void test_ignored_ssl_cert(const uri& base_uri)
    {
        handle_timeout([&base_uri] {
            http_client_config config;
            config.set_validate_certificates(false);
            http_client client(base_uri, config);
            auto response = client.request(methods::GET).get();
            VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
        });
    }
#endif // !defined(__cplusplus_winrt)

    TEST(server_selfsigned_cert) { test_failed_ssl_cert(U("https://self-signed.badssl.com/")); }

#if !defined(__cplusplus_winrt)
    TEST(server_selfsigned_cert_ignored) { test_ignored_ssl_cert(U("https://self-signed.badssl.com/")); }
#endif // !defined(__cplusplus_winrt)

    TEST(server_hostname_mismatch) { test_failed_ssl_cert(U("https://wrong.host.badssl.com/")); }

#if !defined(__cplusplus_winrt) && !defined(CPPREST_FORCE_HTTP_CLIENT_WINHTTPPAL)
    TEST(server_hostname_host_override)
    {
        handle_timeout([] {
            http_client client(U("https://wrong.host.badssl.com/"));
            http_request req(methods::GET);
            req.headers().add(U("Host"), U("badssl.com"));
            auto response = client.request(req).get();
            VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
        });
    }

    TEST(server_hostname_mismatch_ignored) { test_ignored_ssl_cert(U("https://wrong.host.badssl.com/")); }

    TEST(server_hostname_host_override_after_upgrade)
    {
        http_client client(U("http://198.35.26.96/"));
        http_request req(methods::GET);
        req.headers().add(U("Host"), U("en.wikipedia.org"));
        auto response = client.request(req).get();
        VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
    }
#endif // !defined(__cplusplus_winrt) && !defined(CPPREST_FORCE_HTTP_CLIENT_WINHTTPPAL)

    TEST(server_cert_expired) { test_failed_ssl_cert(U("https://expired.badssl.com/")); }

#if !defined(__cplusplus_winrt)
    TEST(server_cert_expired_ignored) { test_ignored_ssl_cert(U("https://expired.badssl.com/")); }
#endif // !defined(__cplusplus_winrt)

    TEST(server_cert_revoked, "Ignore:Android", "229", "Ignore:Apple", "229", "Ignore:Linux", "229")
    {
        test_failed_ssl_cert(U("https://revoked.badssl.com/"));
    }

#if !defined(__cplusplus_winrt)
    TEST(server_cert_revoked_ignored) { test_ignored_ssl_cert(U("https://revoked.badssl.com/")); }
#endif // !defined(__cplusplus_winrt)

    TEST(server_cert_untrusted) { test_failed_ssl_cert(U("https://untrusted-root.badssl.com/")); }

#if !defined(__cplusplus_winrt)
    TEST(server_cert_untrusted_ignored) { test_ignored_ssl_cert(U("https://untrusted-root.badssl.com/")); }
#endif // !defined(__cplusplus_winrt)

#if !defined(__cplusplus_winrt)
    TEST(ignore_server_cert_invalid, "Ignore:Android", "229", "Ignore:Apple", "229", "Ignore:Linux", "229")
    {
        handle_timeout([] {
            http_client_config config;
            config.set_validate_certificates(false);
            config.set_timeout(std::chrono::seconds(1));
            http_client client(U("https://expired.badssl.com/"), config);

            auto request = client.request(methods::GET).get();
            VERIFY_ARE_EQUAL(status_codes::OK, request.status_code());
        });
    }
#endif // !defined(__cplusplus_winrt)
} // SUITE(outside_tests)

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
