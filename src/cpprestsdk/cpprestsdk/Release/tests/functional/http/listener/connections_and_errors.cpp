/***
 * ==++==
 *
 * Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 * ==--==
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * connections_and_errors.cpp
 *
 * Tests cases the underlying connections and error cases with the connection using then http_listener.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include <cpprest/http_client.h>

// For single_core test case.
#if defined(_WIN32) && _MSC_VER < 1900
#include <concrt.h>
#endif

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

using namespace tests::functional::http::utilities;

namespace tests
{
namespace functional
{
namespace http
{
namespace listener
{
SUITE(connections_and_errors)
{
    TEST_FIXTURE(uri_address, close_listener_race, "Ignore", "825350")
    {
        ::http::experimental::listener::http_listener listener(m_uri);
        listener.open().wait();

        listener.support([](http_request) {
            // Let the connection timeout
        });

        // close() racing with a new connection
        auto closeTask = pplx::create_task([&listener]() { listener.close().wait(); });

        auto clientTask = pplx::create_task([this] {
            ::http::client::http_client_config config;
            config.set_timeout(utility::seconds(1));
            ::http::client::http_client client(m_uri, config);

            try
            {
                // Depending on timing this might not succeed. The
                // exception will be caught and ignored below
                auto rsp = client.request(methods::GET).get();

                // The response body should timeout and we should recieve an exception
                rsp.content_ready().wait();

                // If we reach here then it is an error
                VERIFY_IS_FALSE(true);
            }
            catch (std::exception)
            {
            }
        });

        (closeTask && clientTask).wait();
    }

    // Note: Run with admin privileges to listen on default port.
    // This test will fail with "Access denied: attempting to add Address.." exception if it is not run as admin.
    TEST(default_port_close, "Ignore", "Manual")
    {
        uri address(U("http://localhost/portnotspecified"));
        http_listener listener(address);

        try
        {
            listener.open().wait();
        }
        catch (const http_exception& ex)
        {
            VERIFY_IS_FALSE(true, ex.what());
            return;
        }

        // Verify close does not throw an exception while listening on default port
        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, send_response_later)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        std::vector<http_request> requests;
        pplx::extensibility::event_t request_event;
        listener.support([&](http_request r) {
            requests.push_back(r);
            request_event.set();
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::POST, U("")));
        request_event.wait();
        requests[0].reply(status_codes::OK, "HEHEHE").wait();
        requests.clear();
        p_client->next_response()
            .then([&](test_response* p_response) {
                http_asserts::assert_test_response_equals(
                    p_response, status_codes::OK, U("text/plain; charset=utf-8"), U("HEHEHE"));
            })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, save_request_reply)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        http_request request;
        pplx::extensibility::event_t request_event;
        listener.support([&](http_request r) {
            request = r;
            request_event.set();
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::POST, U("")));
        request_event.wait();
        request.reply(status_codes::OK).wait();

        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        listener.close().wait();
    }

#if defined(_WIN32) && _MSC_VER < 1900
    TEST_FIXTURE(uri_address, single_core_request)
    {
        // Fake having a scheduler with only 1 core.
        concurrency::CurrentScheduler::Create(
            concurrency::SchedulerPolicy(2, 1, Concurrency::MinConcurrency, 1, Concurrency::MaxConcurrency));

        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        listener.support([](http_request request) { request.reply(status_codes::OK).get(); });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::POST, U("")));

        // Don't wait on the task otherwise it could inline allowing other tasks to run on the scheduler.
        std::atomic_flag responseEvent = ATOMIC_FLAG_INIT;
        responseEvent.test_and_set();
        p_client->next_response().then([&](test_response* p_response) {
            http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            responseEvent.clear();
        });
        while (responseEvent.test_and_set())
        {
        }

        listener.close().wait();

        concurrency::CurrentScheduler::Detach();
    }
#endif

    TEST_FIXTURE(uri_address, save_request_response)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        http_request request;
        pplx::extensibility::event_t request_event;
        listener.support([&](http_request r) {
            request = r;
            request_event.set();
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::POST, U("")));
        request_event.wait();
        http_response response(status_codes::OK);
        request.reply(response).wait();

        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, reply_twice)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        listener.support([](http_request request) {
            request.reply(status_codes::OK);
            VERIFY_THROWS(request.reply(status_codes::Accepted).get(), http_exception);
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/path")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        listener.close().wait();
    }

    // This test case is manual becuase it requires to be run under and account without admin access.
    TEST(default_port_admin_access, "Ignore", "Manual")
    {
        uri address(U("http://localhost/"));
        http_listener listener(address);
        VERIFY_THROWS(listener.open().wait(), http_exception);
    }

    TEST_FIXTURE(uri_address, try_port_already_in_use, "Ignore:Linux", "Bug 879077", "Ignore:Apple", "Bug 879077")
    {
        test_http_server::scoped_server scoped(m_uri);
        http_listener listener(m_uri);
        VERIFY_THROWS(listener.open().wait(), http_exception);
    }

    TEST_FIXTURE(uri_address, reply_after_starting_close, "Ignore", "901808")
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        listener.support([&](http_request request) {
            // Start closing the listener and then send reply.
            listener.close();
            request.reply(status_codes::OK).wait();
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/path")));

        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
    }

    static void close_stream_early_with_length_impl(const uri& u, bool useException)
    {
        http_listener listener(u);
        listener.open().wait();
        listener.support([=](http_request request) {
            concurrency::streams::producer_consumer_buffer<unsigned char> body;
            concurrency::streams::istream instream = body.create_istream();
            body.putc('A').wait();
            body.putc('B').wait();
            auto responseTask = request.reply(status_codes::OK, instream, 4);

            if (useException)
            {
                body.close(std::ios::out, std::make_exception_ptr(std::invalid_argument("test exception"))).wait();
                VERIFY_THROWS(responseTask.get(), std::invalid_argument);
            }
            else
            {
                body.close(std::ios::out).wait();
                VERIFY_THROWS(responseTask.get(), http_exception);
            }
        });

        web::http::client::http_client client(u);
        client.request(methods::GET, U("/path"))
            .then([](http_response response) -> pplx::task<std::vector<unsigned char>> {
                VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
                return response.extract_vector();
            })
            .then(
                [=](pplx::task<std::vector<unsigned char>> bodyTask) { VERIFY_THROWS(bodyTask.get(), http_exception); })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, close_stream_early_with_length)
    {
        close_stream_early_with_length_impl(m_uri, true);
        close_stream_early_with_length_impl(m_uri, false);
    }

    static void close_stream_early_impl(const uri& u, bool useException)
    {
        http_listener listener(u);
        listener.open().wait();
        listener.support([=](http_request request) {
            concurrency::streams::producer_consumer_buffer<unsigned char> body;
            concurrency::streams::istream instream = body.create_istream();
            body.putc('A').wait();
            body.putc('B').wait();
            auto responseTask = request.reply(status_codes::OK, instream);

            if (useException)
            {
                body.close(std::ios::out, std::make_exception_ptr(std::invalid_argument("test exception"))).wait();
                VERIFY_THROWS(responseTask.get(), std::invalid_argument);
            }
            else
            {
                body.close(std::ios::out).wait();
                responseTask.get();
            }
        });

        web::http::client::http_client client(u);
        client.request(methods::GET, U("/path"))
            .then([](http_response response) -> pplx::task<std::vector<unsigned char>> {
                VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
                return response.extract_vector();
            })
            .then([=](pplx::task<std::vector<unsigned char>> bodyTask) {
                if (useException)
                {
                    VERIFY_THROWS(bodyTask.get(), http_exception);
                }
                else
                {
                    std::vector<unsigned char> body = bodyTask.get();
                    VERIFY_ARE_EQUAL(2, body.size());
                    VERIFY_ARE_EQUAL('A', body[0]);
                    VERIFY_ARE_EQUAL('B', body[1]);
                }
            })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, close_stream_with_exception)
    {
        close_stream_early_impl(m_uri, true);
        close_stream_early_impl(m_uri, false);
    }

    // Helper function to verify http_exception and return the error code value.
    template<typename Func>
    int verify_http_exception(Func f)
    {
        int errorCode = 0;
        try
        {
            f();
            VERIFY_IS_TRUE(false);
        }
        catch (const http_exception& e)
        {
            errorCode = e.error_code().value();
        }
        return errorCode;
    }

    TEST_FIXTURE(uri_address,
                 request_content_ready_timeout,
                 "Ignore:Linux",
                 "Unsuitable until 813276",
                 "Ignore:Apple",
                 "Unsuitable until 813276")
    {
#if !defined(_WIN32) || defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)
        throw std::runtime_error(
            "Unsuitable until 813276 -- http_listener on ASIO does not support timeouts nor chunk sizes");
#endif
        http_listener_config config;
        config.set_timeout(utility::seconds(1));
        http_listener listener(m_uri, config);
        pplx::extensibility::event_t timedOutEvent;
        listener.support([&](http_request req) {
            const int e1 = verify_http_exception([=]() { req.content_ready().wait(); });
            const int e2 = verify_http_exception([=]() { req.body().read().wait(); });
            const int e3 = verify_http_exception([=]() { req.reply(status_codes::OK).wait(); });
            VERIFY_ARE_EQUAL(e1, e2);
            VERIFY_ARE_EQUAL(e2, e3);
            timedOutEvent.set();
        });
        listener.open().wait();

        // Using our production http_client here because it
        // allows separation of sending headers and body.
        ::web::http::client::http_client client(m_uri);
        concurrency::streams::producer_consumer_buffer<unsigned char> body;
        auto responseTask = client.request(methods::PUT, U(""), body.create_istream());
        timedOutEvent.wait();
        body.close().wait();
        VERIFY_THROWS(responseTask.get(), http_exception);

        listener.close().wait();
    }
}

} // namespace listener
} // namespace http
} // namespace functional
} // namespace tests
