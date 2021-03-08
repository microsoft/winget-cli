/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * pipeline_stage_tests.cpp
 *
 * Tests cases using pipeline stages on an http_client.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

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
SUITE(pipeline_stage_tests)
{
    TEST_FIXTURE(uri_address, http_counting_methods)
    {
        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();

        size_t count = 0;

        auto response_counter = [&count](pplx::task<http_response> r_task) -> pplx::task<http_response> {
            ++count;
            return r_task;
        };
        auto request_counter =
            [&count, response_counter](http_request request,
                                       std::shared_ptr<http_pipeline_stage> next_stage) -> pplx::task<http_response> {
            ++count;
            return next_stage->propagate(request).then(response_counter);
        };

        http_client client(m_uri);
        client.add_handler(request_counter);

        // Don't include 'CONNECT' it has a special meaning.
        utility::string_t send_methods[] = {methods::GET,
                                            U("GET"),
                                            methods::DEL,
                                            methods::HEAD,
#ifdef _WIN32 // this is never passed to the listener
                                            methods::OPTIONS,
#endif
                                            methods::POST,
                                            methods::PUT,
                                            methods::PATCH,
                                            U("CUstomMETHOD")};
        utility::string_t recv_methods[] = {U("GET"),
                                            U("GET"),
                                            U("DELETE"),
                                            U("HEAD"),
#ifdef _WIN32
                                            U("OPTIONS"),
#endif
                                            U("POST"),
                                            U("PUT"),
                                            U("PATCH"),
                                            U("CUstomMETHOD")};
        const size_t num_methods = sizeof(send_methods) / sizeof(send_methods[0]);

        for (int i = 0; i < num_methods; ++i)
        {
            p_server->next_request().then([i, &recv_methods](test_request* p_request) {
                http_asserts::assert_test_request_equals(p_request, recv_methods[i], U("/"));
                VERIFY_ARE_EQUAL(0u, p_request->reply(200));
            });
            http_asserts::assert_response_equals(client.request(send_methods[i]).get(), status_codes::OK);
        }

        VERIFY_ARE_EQUAL(num_methods * 2, count);
    }

    TEST_FIXTURE(uri_address, http_short_circuit)
    {
        size_t count = 0;

        auto request_counter = [&count](http_request request,
                                        std::shared_ptr<http_pipeline_stage> next_stage) -> pplx::task<http_response> {
            ++count;
            request.reply(status_codes::Forbidden);
            return request.get_response();
        };

        http_client client(m_uri);
        client.add_handler(request_counter);

        // Don't include 'CONNECT' it has a special meaning.
        utility::string_t send_methods[] = {methods::GET,
                                            U("GET"),
                                            methods::DEL,
                                            methods::HEAD,
                                            methods::OPTIONS,
                                            methods::POST,
                                            methods::PUT,
                                            methods::PATCH,
                                            U("CUstomMETHOD")};
        const size_t num_methods = sizeof(send_methods) / sizeof(send_methods[0]);

        for (int i = 0; i < num_methods; ++i)
        {
            http_asserts::assert_response_equals(client.request(send_methods[i]).get(), status_codes::Forbidden);
        }

        VERIFY_ARE_EQUAL(num_methods, count);
    }

    TEST_FIXTURE(uri_address, http_short_circuit_multiple)
    {
        size_t count = 0;

        auto reply_stage = [](http_request request,
                              std::shared_ptr<http_pipeline_stage> next_stage) -> pplx::task<http_response> {
            request.reply(status_codes::Forbidden);
            return request.get_response();
        };

        auto count_stage = [&count](http_request request,
                                    std::shared_ptr<http_pipeline_stage> next_stage) -> pplx::task<http_response> {
            count++;
            return next_stage->propagate(request);
        };

        http_client client(m_uri);
        client.add_handler(count_stage);
        client.add_handler(count_stage);
        client.add_handler(reply_stage);

        // Don't include 'CONNECT' it has a special meaning.
        utility::string_t send_methods[] = {methods::GET,
                                            U("GET"),
                                            methods::DEL,
                                            methods::HEAD,
                                            methods::OPTIONS,
                                            methods::POST,
                                            methods::PUT,
                                            methods::PATCH,
                                            U("CUstomMETHOD")};
        const size_t num_methods = sizeof(send_methods) / sizeof(send_methods[0]);

        for (int i = 0; i < num_methods; ++i)
        {
            http_asserts::assert_response_equals(client.request(send_methods[i]).get(), status_codes::Forbidden);
        }

        VERIFY_ARE_EQUAL(num_methods * 2, count);
    }

    TEST_FIXTURE(uri_address, http_short_circuit_no_count)
    {
        size_t count = 0;

        auto reply_stage = [](http_request request,
                              std::shared_ptr<http_pipeline_stage> next_stage) -> pplx::task<http_response> {
            request.reply(status_codes::Forbidden);
            return request.get_response();
        };

        auto count_stage = [&count](http_request request,
                                    std::shared_ptr<http_pipeline_stage> next_stage) -> pplx::task<http_response> {
            count++;
            return next_stage->propagate(request);
        };

        // The counting is prevented from happening, because the short-circuit come before the count.
        http_client client(m_uri);
        client.add_handler(reply_stage);
        client.add_handler(count_stage);

        // Don't include 'CONNECT' it has a special meaning.
        utility::string_t send_methods[] = {methods::GET,
                                            U("GET"),
                                            methods::DEL,
                                            methods::HEAD,
                                            methods::OPTIONS,
                                            methods::POST,
                                            methods::PUT,
                                            methods::PATCH,
                                            U("CUstomMETHOD")};
        const size_t num_methods = sizeof(send_methods) / sizeof(send_methods[0]);

        for (int i = 0; i < num_methods; ++i)
        {
            http_asserts::assert_response_equals(client.request(send_methods[i]).get(), status_codes::Forbidden);
        }

        VERIFY_ARE_EQUAL(0u, count);
    }

    /// <summary>
    /// Pipeline stage used for pipeline_stage_inspect_response.
    /// </summary>
    class modify_count_responses_stage : public http_pipeline_stage
    {
    public:
        modify_count_responses_stage() : m_Count(0) {}

        virtual pplx::task<http_response> propagate(http_request request)
        {
            request.headers().set_content_type(U("modified content type"));

            auto currentStage = this->shared_from_this();
            return next_stage()->propagate(request).then([currentStage](http_response response) -> http_response {
                int prevCount = 0;
                response.headers().match(U("My Header"), prevCount);
                utility::stringstream_t data;
                data << prevCount + ++std::dynamic_pointer_cast<modify_count_responses_stage>(currentStage)->m_Count;
                response.headers().add(U("My Header"), data.str());
                return response;
            });
        }

    private:
        int m_Count;
    };

    TEST_FIXTURE(uri_address, pipeline_stage_inspect_response)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        scoped.server()->next_request().then([](test_request* request) {
            http_asserts::assert_test_request_equals(request, methods::GET, U("/"), U("modified content type"));
            request->reply(status_codes::OK);
        });

        // Put in nested scope so we lose the reference on the shared pointer.
        {
            std::shared_ptr<http_pipeline_stage> countStage = std::make_shared<modify_count_responses_stage>();
            client.add_handler(countStage);
            std::shared_ptr<http_pipeline_stage> countStage2 = std::make_shared<modify_count_responses_stage>();
            client.add_handler(countStage2);
        }

        http_response response = client.request(methods::GET).get();
        VERIFY_ARE_EQUAL(status_codes::OK, response.status_code());
        VERIFY_ARE_EQUAL(U("1, 2"), response.headers()[U("My Header")]);
    }

} // SUITE(pipeline_stage_tests)

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
