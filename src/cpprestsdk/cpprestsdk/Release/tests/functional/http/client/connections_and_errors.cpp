/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Tests cases for covering issues dealing with http_client lifetime, underlying TCP connections, and general connection
 *errors.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#ifndef __cplusplus_winrt
#include "cpprest/http_listener.h"
#endif

#include <chrono>
#include <thread>

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
// Test implementation for pending_requests_after_client.
static void pending_requests_after_client_impl(const uri& address)
{
    std::vector<pplx::task<void>> completed_requests;
    {
        test_http_server::scoped_server scoped(address);
        const method mtd = methods::GET;

        const size_t num_requests = 10;

        std::vector<pplx::task<test_request*>> requests = scoped.server()->next_requests(num_requests);
        std::vector<pplx::task<http_response>> responses;
        {
            http_client client(address);

            // send requests.
            for (size_t i = 0; i < num_requests; ++i)
            {
                responses.push_back(client.request(mtd));
            }
        }

        // send responses.
        for (size_t i = 0; i < num_requests; ++i)
        {
            completed_requests.push_back(requests[i].then([&](test_request* request) {
                http_asserts::assert_test_request_equals(request, mtd, U("/"));
                VERIFY_ARE_EQUAL(0u, request->reply(status_codes::OK));
            }));
        }

        // verify responses.
        for (size_t i = 0; i < num_requests; ++i)
        {
            try
            {
                http_asserts::assert_response_equals(responses[i].get(), status_codes::OK);
            }
            catch (...)
            {
                VERIFY_IS_TRUE(false);
            }
        }
    }
    for (auto&& req : completed_requests)
        req.get();
}

SUITE(connections_and_errors)
{
    // Tests requests still outstanding after the http_client has been destroyed.
    TEST_FIXTURE(uri_address, pending_requests_after_client) { pending_requests_after_client_impl(m_uri); }

    TEST_FIXTURE(uri_address, server_doesnt_exist)
    {
        http_client_config config;
        config.set_timeout(std::chrono::seconds(1));
        http_client client(m_uri, config);
        VERIFY_THROWS(client.request(methods::GET).wait(), web::http::http_exception);
    }

    TEST_FIXTURE(uri_address, open_failure)
    {
        http_client client(U("http://localhost323:-1"));

        // This API should not throw. The exception should be surfaced
        // during task.wait/get
        auto t = client.request(methods::GET);
        VERIFY_THROWS(t.wait(), web::http::http_exception);
    }

    TEST_FIXTURE(uri_address, server_close_without_responding)
    {
        http_client_config config;
        config.set_timeout(utility::seconds(1));

        http_client client(m_uri, config);
        test_http_server::scoped_server server(m_uri);
        auto t = server.server()->next_request();

        // Send request.
        auto response = client.request(methods::PUT);

        // Wait for request
        VERIFY_NO_THROWS(t.get());

        // Close server connection.
        server.server()->close();

        VERIFY_THROWS_HTTP_ERROR_CODE(response.wait(), std::errc::connection_aborted);

        // Try sending another request.
        VERIFY_THROWS(client.request(methods::GET).wait(), web::http::http_exception);
    }

    TEST_FIXTURE(uri_address, request_timeout)
    {
        test_http_server::scoped_server scoped(m_uri);
        auto t = scoped.server()->next_request();
        http_client_config config;
        config.set_timeout(utility::seconds(1));

        http_client client(m_uri, config);
        auto responseTask = client.request(methods::GET);

#ifdef __APPLE__
        // CodePlex 295
        VERIFY_THROWS(responseTask.get(), http_exception);
#else
        VERIFY_THROWS_HTTP_ERROR_CODE(responseTask.get(), std::errc::timed_out);
#endif
        t.get();
    }

    TEST_FIXTURE(uri_address, request_timeout_microsecond)
    {
        pplx::task<test_request*> t;
        {
            test_http_server::scoped_server scoped(m_uri);
            t = scoped.server()->next_request();
            http_client_config config;
            config.set_timeout(std::chrono::microseconds(900));

            http_client client(m_uri, config);
            auto responseTask = client.request(methods::GET);
#ifdef __APPLE__
            // CodePlex 295
            VERIFY_THROWS(responseTask.get(), http_exception);
#else
            VERIFY_THROWS_HTTP_ERROR_CODE(responseTask.get(), std::errc::timed_out);
#endif
        }
        try
        {
            t.get();
        }
        catch (...)
        {
        }
    }

    TEST_FIXTURE(uri_address, invalid_method)
    {
        web::http::uri uri(U("http://www.bing.com/"));
        http_client client(uri);
        string_t invalid_chars = U("\a\b\f\v\n\r\t\x20\x7f");

        for (auto iter = invalid_chars.begin(); iter < invalid_chars.end(); iter++)
        {
            string_t method = U("my method");
            method[2] = *iter;
            VERIFY_THROWS(client.request(method).get(), http_exception);
        }
    }

    // This test sends an SSL request to a non-SSL server and should fail on handshaking
    TEST_FIXTURE(uri_address, handshake_fail)
    {
        web::http::uri ssl_uri(U("https://localhost:34568/"));

        test_http_server::scoped_server scoped(m_uri);

        http_client client(ssl_uri);
        auto request = client.request(methods::GET);

        VERIFY_THROWS(request.get(), http_exception);
    }

#if !defined(__cplusplus_winrt)
    TEST_FIXTURE(uri_address, content_ready_timeout)
    {
        web::http::experimental::listener::http_listener listener(m_uri);
        listener.open().wait();

        streams::producer_consumer_buffer<uint8_t> buf;

        listener.support([buf](http_request request) {
            http_response response(200);
            response.set_body(streams::istream(buf), U("text/plain"));
            response.headers().add(header_names::connection, U("close"));
            request.reply(response);
        });

        {
            http_client_config config;
            config.set_timeout(utility::seconds(1));
            http_client client(m_uri, config);
            http_request msg(methods::GET);
            http_response rsp = client.request(msg).get();

            // The response body should timeout and we should receive an exception
            VERIFY_THROWS_HTTP_ERROR_CODE(rsp.content_ready().wait(), std::errc::timed_out);
        }

        buf.close(std::ios_base::out).wait();
        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, stream_timeout)
    {
        web::http::experimental::listener::http_listener listener(m_uri);
        listener.open().wait();

        streams::producer_consumer_buffer<uint8_t> buf;

        listener.support([buf](http_request request) {
            http_response response(200);
            response.set_body(streams::istream(buf), U("text/plain"));
            response.headers().add(header_names::connection, U("close"));
            request.reply(response);
        });

        {
            http_client_config config;
            config.set_timeout(utility::seconds(1));
            http_client client(m_uri, config);
            http_request msg(methods::GET);
            http_response rsp = client.request(msg).get();

            // The response body should timeout and we should receive an exception
            auto readTask = rsp.body().read_to_end(streams::producer_consumer_buffer<uint8_t>());
            VERIFY_THROWS_HTTP_ERROR_CODE(readTask.wait(), std::errc::timed_out);
        }

        buf.close(std::ios_base::out).wait();
        listener.close().wait();
    }
#endif

    TEST_FIXTURE(uri_address, cancel_before_request)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client c(m_uri);
        pplx::cancellation_token_source source;
        source.cancel();

        auto responseTask = c.request(methods::PUT, U("/"), source.get_token());
        VERIFY_THROWS_HTTP_ERROR_CODE(responseTask.get(), std::errc::operation_canceled);
    }

// This test can't be implemented with our test server so isn't available on WinRT.
#ifndef __cplusplus_winrt
    TEST_FIXTURE(uri_address, cancel_after_headers)
    {
        web::http::experimental::listener::http_listener listener(m_uri);
        listener.open().wait();
        http_client c(m_uri);
        pplx::cancellation_token_source source;
        pplx::extensibility::event_t ev;

        listener.support([&](http_request request) {
            streams::producer_consumer_buffer<uint8_t> buf;
            http_response response(200);
            response.set_body(streams::istream(buf), U("text/plain"));
            request.reply(response);
            ev.wait();
            buf.putc('a').wait();
            buf.putc('b').wait();
            buf.putc('c').wait();
            buf.putc('d').wait();
            buf.close(std::ios::out).wait();
        });

        auto responseTask = c.request(methods::GET, source.get_token());
        http_response response = responseTask.get();
        source.cancel();
        ev.set();

        VERIFY_THROWS_HTTP_ERROR_CODE(response.extract_string().get(), std::errc::operation_canceled);

        // Codeplex 328.
#if !defined(_WIN32)
        tests::common::utilities::os_utilities::sleep(1000);
#endif

        listener.close().wait();
    }
#endif

    TEST_FIXTURE(uri_address, cancel_after_body)
    {
        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();
        http_client c(m_uri);
        pplx::cancellation_token_source source;
        std::map<utility::string_t, utility::string_t> headers;
        headers[U("Content-Type")] = U("text/plain; charset=utf-8");
        std::string bodyData("Hello");

        p_server->next_request().then(
            [&](test_request* r) { VERIFY_ARE_EQUAL(0u, r->reply(status_codes::OK, U("OK"), headers, bodyData)); });

        auto response = c.request(methods::PUT, U("/"), U("data"), source.get_token()).get();
        VERIFY_ARE_EQUAL(utility::conversions::to_string_t(bodyData), response.extract_string().get());
        source.cancel();
        response.content_ready().wait();
    }

    TEST_FIXTURE(uri_address, cancel_with_error)
    {
        http_client c(m_uri);
        pplx::task<http_response> responseTask;
        {
            test_http_server::scoped_server server(m_uri);
            pplx::cancellation_token_source source;

            const auto r = server.server()->next_request();
            responseTask = c.request(methods::GET, U("/"), source.get_token());
            r.wait();
            source.cancel();
        }

        // All errors after cancellation are ignored.
        VERIFY_THROWS_HTTP_ERROR_CODE(responseTask.get(), std::errc::operation_canceled);
    }

    TEST_FIXTURE(uri_address, cancel_while_uploading_data)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client c(m_uri);
        pplx::cancellation_token_source source;

        auto buf = streams::producer_consumer_buffer<uint8_t>();
        buf.putc('A').wait();
        auto responseTask = c.request(methods::PUT, U("/"), buf.create_istream(), 2, source.get_token());
        source.cancel();
        buf.putc('B').wait();
        buf.close(std::ios::out).wait();
        VERIFY_THROWS_HTTP_ERROR_CODE(responseTask.get(), std::errc::operation_canceled);
    }

// This test can't be implemented with our test server since it doesn't stream data so isn't avaliable on WinRT.
#ifndef __cplusplus_winrt
    TEST_FIXTURE(uri_address, cancel_while_downloading_data)
    {
        web::http::experimental::listener::http_listener listener(m_uri);
        listener.open().wait();
        http_client c(m_uri);
        pplx::cancellation_token_source source;

        pplx::extensibility::event_t ev;
        pplx::extensibility::event_t ev2;

        listener.support([&](http_request request) {
            streams::producer_consumer_buffer<uint8_t> buf;
            http_response response(200);
            response.set_body(streams::istream(buf), U("text/plain"));
            request.reply(response);
            buf.putc('a').wait();
            buf.putc('b').wait();
            ev.set();
            ev2.wait();
            buf.putc('c').wait();
            buf.putc('d').wait();
            buf.close(std::ios::out).wait();
        });

        auto response = c.request(methods::GET, source.get_token()).get();
        ev.wait();
        source.cancel();
        ev2.set();

        VERIFY_THROWS_HTTP_ERROR_CODE(response.extract_string().get(), std::errc::operation_canceled);

        // Codeplex 328.
#if !defined(_WIN32)
        tests::common::utilities::os_utilities::sleep(1000);
#endif

        listener.close().wait();
    }
#endif

    // Try to connect to a server on a closed port and cancel the operation.
    TEST_FIXTURE(uri_address, cancel_bad_port)
    {
        // http_client_asio had a bug where, when canceled, it would cancel only the
        // current connection but then go and try the next address from the list of
        // resolved addresses, i.e., it wouldn't actually cancel as long as there
        // are more addresses to try. Consequently, it would not report the task as
        // being canceled. This was easiest to observe when trying to connect to a
        // server that does not respond on a certain port, otherwise the timing
        // might be tricky.

        // We need to connect to a URI for which there are multiple addresses
        // associated (i.e., multiple A records).
        web::http::uri uri(U("https://microsoft.com:442/"));

        // Send request.
        http_client_config config;
        config.set_timeout(std::chrono::milliseconds(1000));
        http_client c(uri, config);
        web::http::http_request r;
        auto cts = pplx::cancellation_token_source();
        auto ct = cts.get_token();
        auto t = c.request(r, ct);

        // Make sure that the client already finished resolving before canceling,
        // otherwise the bug might not be triggered.
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        cts.cancel();

        VERIFY_THROWS_HTTP_ERROR_CODE(t.get(), std::errc::operation_canceled);
    }

} // SUITE(connections_and_errors)

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
