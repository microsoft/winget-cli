/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Tests cases for multiple requests and responses from an http_client.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

using namespace web;
using namespace utility;
using namespace utility::conversions;
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
// Helper function to initialize an array of strings to contain 1 MB data.
static void initialize_data(std::string* data_arrays, const size_t count)
{
    // 10k
    std::string data;
    for (int j = 0; j < 1024 * 10; ++j)
    {
        data.push_back('A' + (j % 26));
    }

    for (size_t i = 0; i < count; ++i)
    {
        data_arrays[i] = data;
        data_arrays[i].push_back('a' + (char)i);
    }
}

SUITE(multiple_requests)
{
    TEST_FIXTURE(uri_address, requests_with_data)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client_config config;
        http_client client(m_uri, config);

        const size_t num_requests = 20;
        std::string request_body;
        initialize_data(&request_body, 1);
        const method method = methods::PUT;
        const web::http::status_code code = status_codes::OK;

        std::vector<pplx::task<test_request*>> reqs;
        // response to requests
        for (size_t i = 0; i < num_requests; ++i)
        {
            reqs.push_back(scoped.server()->next_request());
        }

        // send requests
        std::vector<pplx::task<http_response>> responses;
        for (size_t i = 0; i < num_requests; ++i)
        {
            http_request msg(method);
            msg.set_body(request_body);
            responses.push_back(client.request(msg));
        }

        for (auto&& requestTask : reqs)
        {
            auto request = requestTask.get();
            http_asserts::assert_test_request_equals(
                request, method, U("/"), U("text/plain"), to_string_t(request_body));
            VERIFY_ARE_EQUAL(0u, request->reply(code));
        }

        // wait for requests.
        for (size_t i = 0; i < num_requests; ++i)
        {
            try
            {
                http_asserts::assert_response_equals(responses[i].get(), code);
            }
            catch (...)
            {
                VERIFY_ARE_EQUAL(1, 0);
            }
        }
    }

    // Tests multiple requests with responses containing data.
    TEST_FIXTURE(uri_address, responses_with_data)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        const size_t num_requests = 20;
        std::string request_body;
        initialize_data(&request_body, 1);
        const method method = methods::PUT;
        const web::http::status_code code = status_codes::OK;
        std::map<utility::string_t, utility::string_t> headers;
        headers[U("Content-Type")] = U("text/plain");

        // response to requests
        auto requestTasks = scoped.server()->next_requests(num_requests);

        // send requests
        std::vector<pplx::task<http_response>> responses;
        for (size_t i = 0; i < num_requests; ++i)
        {
            responses.push_back(client.request(method));
        }

        // response to requests
        for (size_t i = 0; i < num_requests; ++i)
        {
            test_request* request = requestTasks[i].get();
            http_asserts::assert_test_request_equals(request, method, U("/"));
            VERIFY_ARE_EQUAL(0u, request->reply(code, U(""), headers, request_body));
        }

        // wait for requests.
        for (size_t i = 0; i < num_requests; ++i)
        {
            try
            {
                http_response rsp = responses[i].get();
                http_asserts::assert_response_equals(rsp, code, headers);
                VERIFY_ARE_EQUAL(to_string_t(request_body), rsp.extract_string().get());
            }
            catch (...)
            {
                VERIFY_ARE_EQUAL(1, 0);
            }
        }
    }

} // SUITE(multiple_requests)

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
