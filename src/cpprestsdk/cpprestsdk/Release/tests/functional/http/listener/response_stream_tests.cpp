/***
 * ==++==
 *
 * Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 * ==--==
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * response_stream_tests.cpp
 *
 * Tests cases for streaming with HTTP response with http_listener.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include "cpprest/rawptrstream.h"

using namespace web;
using namespace utility;
using namespace concurrency;
using namespace web::http;
using namespace web::http::experimental::listener;

using namespace tests::common::utilities;
using namespace tests::functional::http::utilities;

namespace tests
{
namespace functional
{
namespace http
{
namespace listener
{
SUITE(response_stream_tests)
{
    // Used to prepare data for read tests
    void fill_file(const utility::string_t& name, size_t repetitions = 1)
    {
        std::fstream stream(name, std::ios_base::out | std::ios_base::trunc);

        for (size_t i = 0; i < repetitions; i++)
            stream << "abcdefghijklmnopqrstuvwxyz";
    }

    TEST_FIXTURE(uri_address, set_body_stream_small)
    {
        utility::string_t fname = U("set_response_stream_small.txt");
        fill_file(fname);

        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        // Try sending data straight from a file.
        http_response response(status_codes::OK);

        auto stream = streams::file_stream<uint8_t>::open_istream(fname).get();
        response.set_body(stream);

        auto length = stream.seek(0, std::ios_base::end);
        stream.seek(0);

        response.headers().set_content_type(U("text/plain; charset=utf-8"));
        response.headers().set_content_length((size_t)length);

        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, methods::POST, U("/"));
            request.reply(response).wait();
        });
        VERIFY_ARE_EQUAL(0u, p_client->request(methods::POST, U("")));
        p_client->next_response()
            .then([&](test_response* p_response) {
                http_asserts::assert_test_response_equals(
                    p_response, status_codes::OK, U("text/plain; charset=utf-8"), U("abcdefghijklmnopqrstuvwxyz"));
            })
            .wait();

        stream.close().get();
    }

    TEST_FIXTURE(uri_address, set_body_stream_large)
    {
        utility::string_t fname = U("set_response_stream_large.txt");
        fill_file(fname, 200);

        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        // Try sending data straight from a file.
        http_response response(status_codes::OK);

        auto stream = streams::file_stream<uint8_t>::open_istream(fname).get();
        response.set_body(stream);

        auto length = stream.seek(0, std::ios_base::end);
        stream.seek(0);

        response.headers().set_content_type(U("text/plain; charset=utf-8"));
        response.headers().set_content_length((size_t)length);

        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, methods::POST, U("/"));
            request.reply(response).wait();
        });
        VERIFY_ARE_EQUAL(0u, p_client->request(methods::POST, U("")));
        p_client->next_response()
            .then([&](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
                VERIFY_ARE_EQUAL((size_t)length, p_response->m_data.size());
            })
            .wait();

        stream.close().get();
    }

    TEST_FIXTURE(uri_address, set_body_stream_partial)
    {
        utility::string_t fname = U("set_response_stream_partial.txt");
        fill_file(fname, 200);

        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        // Try sending data straight from a file.
        http_response response(status_codes::OK);

        auto stream = streams::file_stream<uint8_t>::open_istream(fname).get();
        response.set_body(stream);

        response.headers().set_content_type(U("text/plain; charset=utf-8"));
        response.headers().set_content_length(4500);

        // We shouldn't be sending more than the content-length.

        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, methods::POST, U("/"));
            request.reply(response).wait();
        });
        VERIFY_ARE_EQUAL(0u, p_client->request(methods::POST, U("")));
        p_client->next_response()
            .then([&](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
                VERIFY_ARE_EQUAL(4500, p_response->m_data.size());
            })
            .wait();

        // We should only have read the first 4500 bytes.
        auto length = stream.seek(0, std::ios_base::cur);
        VERIFY_ARE_EQUAL((size_t)length, (size_t)4500);

        stream.close().get();
    }

    TEST_FIXTURE(uri_address, set_body_filestream_chunked)
    {
        utility::string_t fname = U("set_response_stream_chunked.txt");
        fill_file(fname, 200);

        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        // Try sending data straight from a file.
        http_response response(status_codes::OK);

        auto stream = streams::file_stream<uint8_t>::open_istream(fname).get();
        response.set_body(stream);

        auto length = stream.seek(0, std::ios_base::end);
        stream.seek(0);

        response.headers().set_content_type(U("text/plain; charset=utf-8"));
        // Not setting the content length forces "transfer-encoding: chunked"

        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, methods::POST, U("/"));
            request.reply(response).wait();
        });
        VERIFY_ARE_EQUAL(0u, p_client->request(methods::POST, U("")));
        p_client->next_response()
            .then([&](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
                VERIFY_ARE_EQUAL((size_t)length, p_response->m_data.size());
            })
            .wait();

        stream.close().get();
    }

    TEST_FIXTURE(uri_address, set_body_memorystream_chunked)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        // Try sending data straight from a file.
        http_response response(status_codes::OK);

        std::string text1 = "This is a test";
        size_t length = text1.size();

        response.headers().set_content_type(U("text/plain; charset=utf-8"));
        // Not setting the content length forces "transfer-encoding: chunked"

        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, methods::POST, U("/"));

            streams::producer_consumer_buffer<char> rwbuf;

            streams::basic_istream<uint8_t> stream(rwbuf);
            response.set_body(stream);

            auto rep = request.reply(response);

            os_utilities::sleep(100);

            rwbuf.putn_nocopy(&text1[0], length).wait();
            rwbuf.putn_nocopy(&text1[0], length).wait();
            rwbuf.sync().wait();
            rwbuf.putn_nocopy(&text1[0], length).wait();
            rwbuf.close(std::ios_base::out).wait();

            rep.wait();
        });

        VERIFY_ARE_EQUAL(0u, p_client->request(methods::POST, U("")));
        p_client->next_response()
            .then([&](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
                VERIFY_ARE_EQUAL((size_t)length * 3, p_response->m_data.size());
            })
            .wait();
    }

    TEST_FIXTURE(uri_address, reply_transfer_encoding_4k)
    {
        web::http::experimental::listener::http_listener listener(m_uri);
        listener.open().wait();

        streams::container_buffer<std::vector<uint8_t>> buf;

        // Write 4K - the exact internal chunk size
        unsigned char ptr[4 * 1024] = {'a', 'b', 'c'};
        VERIFY_ARE_EQUAL(buf.putn_nocopy(ptr, sizeof(ptr)).get(), sizeof(ptr));

        listener.support([&buf](http_request request) {
            // Ensure that it is transfer-encoded
            auto collection = buf.collection();
            streams::container_buffer<std::vector<uint8_t>> buf2(std::move(collection), std::ios_base::in);
            request.reply(200, streams::istream(buf2), U("text/plain"));
            buf.close(std::ios_base::out);
        });

        {
            ::http::client::http_client client(m_uri);
            http_request msg(methods::GET);

            // Wait for headers
            auto resp = client.request(msg).get();

            // Wait for data
            resp.content_ready().wait();

            // Now verify that we've got the right data
            auto s = resp.extract_string().get();
            VERIFY_ARE_EQUAL(s.c_str(), U("abc"));
        }
        listener.close().wait();
    }

    // Fails sporadically, Codeplex #158
    TEST_FIXTURE(uri_address, reply_chunked_4k, "Ignore", "Codeplex 158")
    {
        web::http::experimental::listener::http_listener listener(m_uri);
        listener.open().wait();

        streams::producer_consumer_buffer<uint8_t> buf;

        // Write 4K - the exact internal chunk size
        unsigned char ptr[4 * 1024];
        VERIFY_ARE_EQUAL(buf.putn_nocopy(ptr, sizeof(ptr)).get(), sizeof(ptr));
        buf.close(std::ios_base::out);

        listener.support([&buf](http_request request) {
            // Ensure that it is transfer-encoded
            request.reply(200, streams::istream(buf), 4096, U("text/plain"));
        });

        {
            ::http::client::http_client client(m_uri);
            http_request msg(methods::GET);

            // Wait for headers
            auto resp = client.request(msg).get();

            // Wait for data
            resp.content_ready().wait();
        }
        listener.close().wait();
    }
}

} // namespace listener
} // namespace http
} // namespace functional
} // namespace tests
