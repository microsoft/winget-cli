/***
 * ==++==
 *
 * Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 * ==--==
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * request_stream_tests.cpp
 *
 * Tests cases for streaming HTTP requests with http_listener.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

using namespace web;
using namespace utility;
using namespace concurrency;
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
SUITE(request_stream_tests)
{
    TEST_FIXTURE(uri_address, large_body)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        std::string data_piece("abcdefghijklmnopqrstuvwxyz");
        std::string send_data;
        // 26 * 160 is greater than 4k which is the chunk size.
        for (int i = 0; i < 160; ++i)
        {
            send_data.append(data_piece);
        }
        size_t length = send_data.size();

        listener.support([&](http_request request) {
            auto stream = request.body();
            streams::stringstreambuf strbuf;

            VERIFY_ARE_EQUAL(stream.read_to_end(strbuf).get(), length);
            VERIFY_ARE_EQUAL(strbuf.collection(), send_data);
            request.reply(status_codes::OK);
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U(""), U("text/plain"), send_data));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, test_chunked_transfer)
    {
        const size_t num_bytes = 1024 * 1024 * 10;
        http_listener listener(m_uri);
        listener.support([num_bytes](http_request request) { request.reply(status_codes::OK); });
        listener.open().wait();

        ::http::client::http_client client(m_uri);
        auto buf = streams::producer_consumer_buffer<uint8_t>();
        pplx::task<http_response> response =
            client.request(methods::PUT, U("/"), buf.create_istream(), U("text/plain"));

        const size_t four_mb = 1024 * 1024 * 4;
        std::vector<uint8_t> buffer;
        buffer.resize(num_bytes);
        memset(&buffer[0], (int)'A', num_bytes);
        size_t start = 0, end;
        while (start < num_bytes)
        {
            end = start + four_mb < num_bytes ? four_mb : num_bytes - start;
            size_t num_written = buf.putn_nocopy(&buffer[start], end).get();
            start += num_written;
        }
        buf.close(std::ios_base::out).wait();

        response.wait();
        listener.close().wait();
    }
}

} // namespace listener
} // namespace http
} // namespace functional
} // namespace tests
