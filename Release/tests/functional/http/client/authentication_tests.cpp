/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Tests cases for authentication with http_clients.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include <stdexcept>

#ifdef _WIN32
#ifdef __cplusplus_winrt
#if !defined(__WRL_NO_DEFAULT_LIB__)
#define __WRL_NO_DEFAULT_LIB__
#endif
#include <msxml6.h>
#include <wrl.h>
#else
#include <windows.h>

#include <winhttp.h>
#pragma comment(lib, "winhttp")
#endif
#endif

#if !defined(_WIN32)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winfinite-recursion"
#endif
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#endif

using namespace web;
using namespace utility;
using namespace concurrency;
using namespace web::http;
using namespace web::http::client;

using namespace tests::functional::http::utilities;

namespace tests
{
namespace functional
{
namespace http
{
namespace client
{
SUITE(authentication_tests)
{
    TEST_FIXTURE(uri_address, auth_no_data, "Ignore:Linux", "89", "Ignore:Apple", "89")
    {
        pplx::task<void> t, t2;
        {
            test_http_server::scoped_server scoped(m_uri);
            http_client_config client_config;
            web::credentials cred(U("some_user"), U("some_password")); // WinHTTP requires non-empty password
            client_config.set_credentials(cred);
            http_client client(m_uri, client_config);
            const method mtd = methods::POST;

            http_request msg(mtd);

            t = scoped.server()->next_request().then([&](test_request* p_request) {
                http_asserts::assert_test_request_equals(p_request, mtd, U("/"));

                // Auth header
                std::map<utility::string_t, utility::string_t> headers;
                headers[U("WWW-Authenticate")] = U("Basic realm = \"WallyWorld\"");

                // unauthorized
                p_request->reply(status_codes::Unauthorized, U("Authentication Failed"), headers);
            });
            t2 = scoped.server()->next_request().then([&](test_request* p_request) {
                http_asserts::assert_test_request_equals(p_request, methods::POST, U("/"));
                p_request->reply(200);
            });

            try
            {
                http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);
            }
            catch (...)
            {
                VERIFY_ARE_EQUAL(0, 1);
            }
        }
        try
        {
            t.get();
        }
        catch (...)
        {
            VERIFY_ARE_EQUAL(0, 1);
        }
        try
        {
            t2.get();
        }
        catch (...)
        {
            VERIFY_ARE_EQUAL(0, 1);
        }
    }

// TFS 648783
#ifndef __cplusplus_winrt
    TEST_FIXTURE(uri_address, proxy_auth_known_contentlength, "Ignore:Linux", "88", "Ignore:Apple", "88")
    {
        pplx::task<void> t, t2;
        {
            test_http_server::scoped_server scoped(m_uri);
            http_client_config client_config;
            web::credentials cred(U("some_user"), U("some_password")); // WinHTTP requires non-empty password
            client_config.set_credentials(cred);
            http_client client(m_uri, client_config);
            const method mtd = methods::POST;
            utility::string_t contents(U("Hello World"));

            http_request msg(mtd);
            msg.set_body(contents);

            t = scoped.server()->next_request().then([&](test_request* p_request) {
                http_asserts::assert_test_request_equals(p_request, mtd, U("/"));

                // Auth header
                std::map<utility::string_t, utility::string_t> headers;
                headers[U("WWW-Authenticate")] = U("Basic realm = \"WallyWorld\"");

                // unauthorized
                p_request->reply(status_codes::Unauthorized, U("Authentication Failed"), headers);
            });

            t2 = scoped.server()->next_request().then([&](test_request* p_request) {
                http_asserts::assert_test_request_equals(
                    p_request, methods::POST, U("/"), U("text/plain; charset=utf-8"), contents);

                p_request->reply(200);
            });

            http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);
        }
        try
        {
            t.get();
        }
        catch (...)
        {
            VERIFY_ARE_EQUAL(0, 1);
        }
        try
        {
            t2.get();
        }
        catch (...)
        {
            VERIFY_ARE_EQUAL(0, 1);
        }
    }
#endif

    TEST_FIXTURE(uri_address, proxy_auth_noseek, "Ignore:Linux", "88", "Ignore:Apple", "88")
    {
        web::http::uri uri(U("http://localhost:34567/"));
        test_http_server::scoped_server scoped(uri);
        http_client client(
            uri); // In this test, the request cannot be resent, so the username and password are not required
        const method mtd = methods::POST;

        auto buf = streams::producer_consumer_buffer<unsigned char>();
        buf.putc('a').get();
        buf.close(std::ios_base::out).get();

        http_request msg(mtd);
        msg.set_body(buf.create_istream(), 1);

        scoped.server()->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, mtd, U("/"));

            // Auth header
            std::map<utility::string_t, utility::string_t> headers;
            headers[U("WWW-Authenticate")] = U("Basic realm = \"WallyWorld\"");

            // unauthorized
            p_request->reply(status_codes::Unauthorized, U("Authentication Failed"), headers);
        });

        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::Unauthorized);
    }

// Must specify content length with winrt client, so this test case isn't possible.
#ifndef __cplusplus_winrt
    TEST_FIXTURE(uri_address, proxy_auth_unknown_contentlength, "Ignore:Linux", "88", "Ignore:Apple", "88")
    {
        pplx::task<void> t;
        {
            test_http_server::scoped_server scoped(m_uri);
            http_client_config client_config;
            web::credentials cred(U("some_user"), U("some_password")); // WinHTTP requires non-empty password
            client_config.set_credentials(cred);
            http_client client(m_uri, client_config);
            const method mtd = methods::POST;

            std::vector<uint8_t> msg_body;
            msg_body.push_back('a');

            http_request msg(mtd);
            msg.set_body(streams::container_stream<std::vector<uint8_t>>::open_istream(std::move(msg_body)));

            auto replyFunc = [&](test_request* p_request) {
                utility::string_t contents(U("a"));
                http_asserts::assert_test_request_equals(
                    p_request, methods::POST, U("/"), U("application/octet-stream"), contents);

                p_request->reply(200);
            };

            t = scoped.server()
                    ->next_request()
                    .then([&](test_request* p_request) {
                        http_asserts::assert_test_request_equals(p_request, mtd, U("/"));

                        // Auth header
                        std::map<utility::string_t, utility::string_t> headers;
                        headers[U("WWW-Authenticate")] = U("Basic realm = \"WallyWorld\"");

                        // unauthorized
                        p_request->reply(status_codes::Unauthorized, U("Authentication Failed"), headers);
                    })
                    .then([&scoped, replyFunc]() {
                        // Client resent the request
                        return scoped.server()->next_request().then(replyFunc);
                    });

            http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);
        }
        t.get();
    }

    // Accessing a server that returns 401 with an empty user name should not resend the request with an empty password
    TEST_FIXTURE(uri_address, empty_username_password)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        auto t = scoped.server()->next_request().then([&](test_request* p_request) {
            std::map<utility::string_t, utility::string_t> headers;
            headers[U("h1")] = U("data1");
            // Auth header
            headers[U("WWW-Authenticate")] = U("Basic realm = \"myRealm\"");
            // unauthorized
            p_request->reply(status_codes::Unauthorized, U("Authentication Failed"), headers, "a");
        });

        http_response response = client.request(methods::GET).get();
        auto str_body = response.extract_vector().get();
        auto h1 = response.headers()[U("h1")];
        VERIFY_ARE_EQUAL(status_codes::Unauthorized, response.status_code());
        VERIFY_ARE_EQUAL(str_body[0], 'a');
        VERIFY_ARE_EQUAL(h1, U("data1"));
        t.get();
    }
#endif

    // Fails on WinRT due to TFS 648278
    // Accessing a server that supports auth, but returns 401, even after the user has provided valid creds
    // We're making sure the error is reported properly, and the response data from the second response is received
    TEST_FIXTURE(uri_address, error_after_valid_credentials, "Ignore:Linux", "89", "Ignore:Apple", "89")
    {
        pplx::task<void> t;
        {
            web::http::uri uri(U("http://localhost:34569/"));
            test_http_server::scoped_server scoped(uri);
            http_client_config client_config;
            web::credentials cred(U("some_user"), U("some_password"));
            client_config.set_credentials(cred);
            http_client client(uri, client_config);

            auto replyFunc = [&](test_request* p_request) {
                std::map<utility::string_t, utility::string_t> headers;
                // Auth header
                headers[U("WWW-Authenticate")] = U("Basic realm = \"WallyWorld\"");
                headers[U("h1")] = U("data2");
                // still unauthorized after the user has resent the request with the credentials
                p_request->reply(status_codes::Unauthorized, U("Authentication Failed"), headers, "def");
            };

            t = scoped.server()
                    ->next_request()
                    .then([&](test_request* p_request) {
                        std::map<utility::string_t, utility::string_t> headers;
                        headers[U("h1")] = U("data1");
                        // Auth header
                        headers[U("WWW-Authenticate")] = U("Basic realm = \"myRealm\"");
                        // unauthorized
                        p_request->reply(status_codes::Unauthorized, U("Authentication Failed"), headers, "abc");
                    })
                    .then([&scoped, &replyFunc]() {
                        // Client resent the request
                        return scoped.server()->next_request().then(replyFunc);
                    })
#ifdef __cplusplus_winrt
                    .then([&scoped, &replyFunc]() {
                        // in winrt, client resent the request again
                        return scoped.server()->next_request().then(replyFunc);
                    })
#endif
                ;

            http_response response = client.request(methods::GET).get();
            auto str_body = response.extract_vector().get();
            auto h1 = response.headers()[U("h1")];
            VERIFY_ARE_EQUAL(status_codes::Unauthorized, response.status_code());
            VERIFY_ARE_EQUAL(str_body[0], 'd');
            VERIFY_ARE_EQUAL(str_body[1], 'e');
            VERIFY_ARE_EQUAL(str_body[2], 'f');
            VERIFY_ARE_EQUAL(h1, U("data2"));
        }
        t.get();
    }

    // These tests are disabled since they require a server with authentication running.
    // The server portion to use is the C# AuthenticationListener.

    class server_properties
    {
    public:
        server_properties() {}

        // Helper function to retrieve all parameters necessary for setup tests.
        void load_parameters()
        {
            m_uri = uri(utility::conversions::to_string_t(UnitTest::GlobalSettings::Get("Server")));
            if (UnitTest::GlobalSettings::Has("UserName"))
            {
                m_username = utility::conversions::to_string_t(UnitTest::GlobalSettings::Get("UserName"));
            }
            if (UnitTest::GlobalSettings::Has("Password"))
            {
                m_password = utility::conversions::to_string_t(UnitTest::GlobalSettings::Get("Password"));
            }
        }

        web::http::uri m_uri;
        string_t m_username;
        string_t m_password;
    };

    // This test should be executed for NTLM, Negotiate, IntegratedWindowsAuth, and Anonymous.
    TEST_FIXTURE(server_properties, successful_auth_no_cred, "Requires", "Server")
    {
        load_parameters();

        http_client client(m_uri);
        http_response response = client.request(methods::GET).get();
        VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
    }

    TEST_FIXTURE(server_properties, digest_basic_auth_no_cred, "Requires", "Server")
    {
        load_parameters();

        http_client client(m_uri);
        http_response response = client.request(methods::GET).get();
        VERIFY_ARE_EQUAL(status_codes::Unauthorized, response.status_code());
    }

    TEST_FIXTURE(server_properties, none_auth_no_cred, "Requires", "Server")
    {
        load_parameters();

        http_client client(m_uri);
        http_response response = client.request(methods::GET).get();
        VERIFY_ARE_EQUAL(status_codes::Forbidden, response.status_code());
    }

    // This test should be executed for NTLM, Negotiate, IntegratedWindowsAuth, and Digest.
    TEST_FIXTURE(server_properties, unsuccessful_auth_with_basic_cred, "Requires", "Server;UserName;Password")
    {
        load_parameters();

        http_client_config config;
        config.set_credentials(web::credentials(m_username, m_password));

        http_client client(m_uri, config);
        http_response response = client.request(methods::GET).get();
        VERIFY_ARE_EQUAL(status_codes::Unauthorized, response.status_code());
    }

    TEST_FIXTURE(server_properties, basic_anonymous_auth_with_basic_cred, "Requires", "Server;UserName;Password")
    {
        load_parameters();

        http_client_config config;
        config.set_credentials(web::credentials(m_username, m_password));
        http_client client(m_uri, config);
        http_request req(methods::GET);
        req.headers().add(U("UserName"), m_username);
        req.headers().add(U("Password"), m_password);
        http_response response = client.request(req).get();
        VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
    }

    TEST_FIXTURE(server_properties, none_auth_with_cred, "Requires", "Server;UserName;Password")
    {
        load_parameters();

        http_client_config config;
        config.set_credentials(web::credentials(m_username, m_password));
        http_client client(m_uri, config);
        http_response response = client.request(methods::GET).get();
        VERIFY_ARE_EQUAL(status_codes::Forbidden, response.status_code());
    }

    // This test should be executed for all authentication schemes except None.
    TEST_FIXTURE(server_properties, successful_auth_with_domain_cred, "Requires", "Server;UserName;Password")
    {
        load_parameters();

        http_client_config config;
        config.set_credentials(web::credentials(m_username, m_password));
        http_client client(m_uri, config);
        http_request req(methods::GET);
        req.headers().add(U("UserName"), m_username);
        req.headers().add(U("Password"), m_password);
        http_response response = client.request(req).get();
        VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
    }

#ifndef __cplusplus_winrt // WinRT implementation doesn't support request buffer caching.
    TEST_FIXTURE(server_properties, failed_authentication_resend_request_error, "Requires", "Server;UserName;Password")
    {
        load_parameters();

        http_client_config config;
        config.set_credentials(web::credentials(m_username, m_password));
        http_client client(m_uri, config);

        const size_t rawDataSize = 8;

        std::vector<unsigned char> data(rawDataSize);
        memcpy(&data[0], "raw data", rawDataSize);

        http_request request;
        request.set_method(methods::POST);
        request.set_body(data);
        http_response response = client.request(request).get();

        VERIFY_ARE_EQUAL(200, response.status_code());
    }
#endif

#ifdef __cplusplus_winrt
    TEST_FIXTURE(uri_address, set_user_options_winrt)
    {
        test_http_server::scoped_server scoped(m_uri);
        scoped.server()->next_request().then([](test_request* p_request) { p_request->reply(status_codes::OK); });

        http_client_config config;
        config.set_nativehandle_options([](native_handle handle) -> void {
            auto hr = handle->SetProperty(XHR_PROP_TIMEOUT, 1000);
            if (!SUCCEEDED(hr)) throw std::runtime_error("The Test Exception");
        });
        http_client client(m_uri, config);
        auto response = client.request(methods::GET).get();
        VERIFY_ARE_EQUAL(200, response.status_code());
    }
#endif // __cplusplus_winrt

#ifdef _WIN32
#if !defined(__cplusplus_winrt)
    TEST_FIXTURE(server_properties, set_user_options, "Requires", "Server;UserName;Password")
    {
        load_parameters();

        http_client_config config;
        config.set_credentials(web::credentials(m_username, m_password));

        config.set_nativehandle_options([&](native_handle handle) -> void {
            DWORD policy = WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW;
            if (!WinHttpSetOption(handle, WINHTTP_OPTION_AUTOLOGON_POLICY, &policy, sizeof(policy)))
            {
                throw std::runtime_error("The Test Error");
            }
        });

        http_client client(m_uri, config);

        const size_t rawDataSize = 8;

        std::vector<unsigned char> data(rawDataSize);
        memcpy(&data[0], "raw data", rawDataSize);

        http_request request;
        request.set_method(methods::POST);
        request.set_body(data);

        VERIFY_ARE_EQUAL(200, client.request(request).get().status_code());
    }

    TEST_FIXTURE(uri_address, auth_producer_consumer_buffer)
    {
        auto buf = streams::producer_consumer_buffer<unsigned char>();
        buf.putc('a').get();
        buf.putc('a').get();
        buf.putc('a').get();
        buf.putc('a').get();
        buf.close(std::ios_base::out).get();
        http_request msg(methods::POST);
        msg.set_body(buf.create_istream());

        http_client_config config;
        VERIFY_IS_FALSE(config.buffer_request());
        config.set_buffer_request(true);
        VERIFY_IS_TRUE(config.buffer_request());
        config.set_credentials(web::credentials(U("USERNAME"), U("PASSWORD")));

        http_client client(m_uri, config);

        pplx::task<void> t, t2;
        test_http_server::scoped_server scoped(m_uri);

        t = scoped.server()->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(
                p_request, methods::POST, U("/"), U("application/octet-stream"), U("aaaa"));
            std::map<utility::string_t, utility::string_t> headers;
            headers[U("WWW-Authenticate")] = U("Basic realm = \"WallyWorld\"");

            p_request->reply(status_codes::Unauthorized, U("Authentication Failed"), headers);
        });
        t2 = scoped.server()->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(
                p_request, methods::POST, U("/"), U("application/octet-stream"), U("aaaa"));
            p_request->reply(200);
        });

        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);
        scoped.server()->close();
        VERIFY_NO_THROWS(t.get());
        VERIFY_NO_THROWS(t2.get());
    }

    TEST_FIXTURE(uri_address, auth_producer_comsumer_buffer_fail_no_cred)
    {
        auto buf = streams::producer_consumer_buffer<unsigned char>();
        buf.putc('a').get();
        buf.putc('a').get();
        buf.putc('a').get();
        buf.putc('a').get();
        buf.close(std::ios_base::out).get();
        http_request msg(methods::POST);
        msg.set_body(buf.create_istream());

        http_client client(m_uri);

        pplx::task<void> t;
        {
            test_http_server::scoped_server scoped(m_uri);
            t = scoped.server()->next_request().then([&](test_request* p_request) {
                http_asserts::assert_test_request_equals(
                    p_request, methods::POST, U("/"), U("application/octet-stream"), U("aaaa"));
                std::map<utility::string_t, utility::string_t> headers;
                headers[U("WWW-Authenticate")] = U("Basic realm = \"WallyWorld\"");

                p_request->reply(status_codes::Unauthorized, U("Authentication Failed"), headers);
            });

            http_asserts::assert_response_equals(client.request(msg).get(), status_codes::Unauthorized);
        }
        t.get();
    }

    TEST_FIXTURE(uri_address, auth_producer_comsumer_buffer_fail)
    {
        auto buf = streams::producer_consumer_buffer<unsigned char>();
        buf.putc('a').get();
        buf.close(std::ios_base::out).get();
        http_request msg(methods::POST);
        msg.set_body(buf.create_istream());

        http_client_config config;
        config.set_buffer_request(true);
        config.set_credentials(web::credentials(U("USERNAME"), U("PASSWORD")));

        http_client client(m_uri, config);
        pplx::task<void> t;
        {
            test_http_server::scoped_server scoped(m_uri);

            auto replyFunc = [&](test_request* p_request) {
                http_asserts::assert_test_request_equals(
                    p_request, methods::POST, U("/"), U("application/octet-stream"), U("a"));
                std::map<utility::string_t, utility::string_t> headers;
                headers[U("WWW-Authenticate")] = U("Basic realm = \"WallyWorld2\"");

                p_request->reply(status_codes::Unauthorized, U("Authentication Failed"), headers);
            };

            t = scoped.server()
                    ->next_request()
                    .then([&](test_request* p_request) {
                        http_asserts::assert_test_request_equals(
                            p_request, methods::POST, U("/"), U("application/octet-stream"), U("a"));
                        std::map<utility::string_t, utility::string_t> headers;
                        headers[U("WWW-Authenticate")] = U("Basic realm = \"WallyWorld\"");

                        p_request->reply(status_codes::Unauthorized, U("Authentication Failed"), headers);
                    })
                    .then([&scoped, replyFunc]() { return scoped.server()->next_request().then(replyFunc); });

            http_asserts::assert_response_equals(client.request(msg).get(), status_codes::Unauthorized);
        }
        VERIFY_NO_THROWS(t.get());
    }
#endif

    TEST_FIXTURE(uri_address, set_user_options_exceptions)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client_config config;
        class TestException;
        config.set_nativehandle_options([](native_handle) { throw std::runtime_error("The Test exception"); });
        http_client client(m_uri, config);
        VERIFY_THROWS(client.request(methods::GET).get(), std::runtime_error);
    }
#endif // _WIN32

    // Fix for 522831 AV after failed authentication attempt
    TEST_FIXTURE(uri_address, failed_authentication_attempt, "Ignore:Linux", "89", "Ignore:Apple", "89")
    {
        handle_timeout([] {
            http_client_config config;
            web::credentials cred(U("user"), U("schmuser"));
            config.set_credentials(cred);
            http_client client(U("https://apis.live.net"), config);
            http_response response = client.request(methods::GET, U("V5.0/me/skydrive/files")).get();
            VERIFY_ARE_EQUAL(status_codes::Unauthorized, response.status_code());
            auto v = response.extract_vector().get();
            std::string s(v.begin(), v.end());
            // The resulting data must be non-empty (an error about missing access token)
            VERIFY_IS_FALSE(s.empty());
        });
    }

#if !defined(_WIN32)

    // http_server does not support auth
    void auth_test_impl(bool fail)
    {
        std::string user("user1"), password("user1");
        auto return_code = status_codes::OK;

        if (fail)
        {
            password = "invalid";
            return_code = status_codes::Unauthorized;
        }

        http_client_config client_config;
        web::credentials cred(U(user), U(password));
        client_config.set_credentials(cred);
        http_client client(U("http://httpbin.org/basic-auth/user1/user1"), client_config);

        http_response response = client.request(methods::GET).get();
        VERIFY_ARE_EQUAL(return_code, response.status_code());
    }

    TEST(auth_no_data) { auth_test_impl(false); }

    TEST(unsuccessful_auth_with_basic_cred) { auth_test_impl(true); }

    TEST_FIXTURE(uri_address, set_user_options_asio_http)
    {
        test_http_server::scoped_server scoped(m_uri);
        scoped.server()->next_request().then([](test_request* p_request) { p_request->reply(status_codes::OK); });

        http_client_config config;
        config.set_nativehandle_options([](native_handle handle) {
            boost::asio::ip::tcp::socket* socket = static_cast<boost::asio::ip::tcp::socket*>(handle);
            // Socket shouldn't be open yet since no requests have gone out.
            VERIFY_ARE_EQUAL(false, socket->is_open());
        });
        http_client client(m_uri, config);
        auto response = client.request(methods::GET).get();
        VERIFY_ARE_EQUAL(200, response.status_code());
    }

    TEST_FIXTURE(uri_address, set_user_options_asio_https)
    {
        handle_timeout([] {
            http_client_config config;
            config.set_nativehandle_options([](native_handle handle) {
                boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>* streamobj =
                    static_cast<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>*>(handle);
                const auto& tcpLayer = streamobj->lowest_layer();
                VERIFY_ARE_EQUAL(false, tcpLayer.is_open());
            });

            http_client client(U("https://apis.live.net"), config);
            http_response response = client.request(methods::GET, U("V5.0/me/skydrive/files")).get();
            VERIFY_ARE_EQUAL(status_codes::Unauthorized, response.status_code());
            auto v = response.extract_vector().get();
            // The resulting data must be non-empty (an error about missing access token)
            VERIFY_IS_FALSE(v.empty());
        });
    }

#endif

} // SUITE(authentication_tests)

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
