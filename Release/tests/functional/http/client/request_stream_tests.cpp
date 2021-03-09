/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Tests cases covering using streams with HTTP request with http_client.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#if defined(__cplusplus_winrt)
using namespace Windows::Storage;
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
utility::string_t get_full_name(const utility::string_t& name)
{
#if defined(__cplusplus_winrt)
    // On WinRT, we must compensate for the fact that we will be accessing files in the
    // Documents folder
    auto file = pplx::create_task(KnownFolders::DocumentsLibrary->CreateFileAsync(
                                      ref new Platform::String(name.c_str()), CreationCollisionOption::ReplaceExisting))
                    .get();
    return file->Path->Data();
#else
    return name;
#endif
}

template<typename _CharType>
pplx::task<streams::streambuf<_CharType>> OPEN_R(const utility::string_t& name)
{
#if !defined(__cplusplus_winrt)
    return streams::file_buffer<_CharType>::open(name, std::ios_base::in);
#else
    auto file =
        pplx::create_task(KnownFolders::DocumentsLibrary->GetFileAsync(ref new Platform::String(name.c_str()))).get();

    return streams::file_buffer<_CharType>::open(file, std::ios_base::in);
#endif
}

SUITE(request_stream_tests)
{
    // Used to prepare data for stream tests
    void fill_file(const utility::string_t& name, size_t repetitions = 1)
    {
        std::fstream stream(get_full_name(name), std::ios_base::out | std::ios_base::trunc);

        for (size_t i = 0; i < repetitions; i++)
            stream << "abcdefghijklmnopqrstuvwxyz";
    }

    void fill_buffer(streams::streambuf<uint8_t> rbuf, size_t repetitions = 1)
    {
        const char* text = "abcdefghijklmnopqrstuvwxyz";
        size_t len = strlen(text);
        for (size_t i = 0; i < repetitions; i++)
            rbuf.putn_nocopy((const uint8_t*)text, len);
    }

#if defined(__cplusplus_winrt)
    TEST_FIXTURE(uri_address, ixhr2_transfer_encoding)
    {
        // Transfer encoding chunked is not supported. Not specifying the
        // content length should cause an exception from the task. Verify
        // that there is no unobserved exception

        http_client client(m_uri);

        auto buf = streams::producer_consumer_buffer<uint8_t>();
        buf.putc(22).wait();
        buf.close(std::ios_base::out).wait();

        http_request reqG(methods::PUT);
        reqG.set_body(buf.create_istream());
        VERIFY_THROWS(client.request(reqG).get(), http_exception);

        VERIFY_THROWS(client.request(methods::POST, U(""), buf.create_istream(), 1).get(), http_exception);
    }
#endif

    TEST_FIXTURE(uri_address, set_body_stream_1)
    {
        utility::string_t fname = U("set_body_stream_1.txt");
        fill_file(fname);

        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();
        http_client client(m_uri);

        auto stream = OPEN_R<uint8_t>(fname).get();
        http_request msg(methods::POST);
        msg.set_body(stream);
#if defined(__cplusplus_winrt)
        msg.headers().set_content_length(26);
#endif
        p_server->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, methods::POST, U("/"));
            VERIFY_ARE_EQUAL(26u, p_request->m_body.size());
            std::string str_body(std::begin(p_request->m_body), std::end(p_request->m_body));
            VERIFY_ARE_EQUAL(U("abcdefghijklmnopqrstuvwxyz"), ::utility::conversions::to_string_t(str_body));
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);
        stream.close().wait();
    }

    TEST_FIXTURE(uri_address, set_body_stream_2)
    {
        utility::string_t fname = U("set_body_stream_2.txt");
        fill_file(fname);

        http_client_config config;
        config.set_chunksize(16 * 1024);

        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();
        http_client client(m_uri, config);

        auto stream = OPEN_R<uint8_t>(fname).get();
        http_request msg(methods::POST);
        msg.set_body(stream);
#if defined(__cplusplus_winrt)
        msg.headers().set_content_length(26);
#endif
        p_server->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, methods::POST, U("/"));
            VERIFY_ARE_EQUAL(26u, p_request->m_body.size());
            std::string str_body(std::begin(p_request->m_body), std::end(p_request->m_body));
            VERIFY_ARE_EQUAL(U("abcdefghijklmnopqrstuvwxyz"), ::utility::conversions::to_string_t(str_body));
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);
        stream.close().wait();
    }

    // Implementation for request with stream test case.
    static void stream_request_impl(
        const uri& address, bool withContentLength, size_t chunksize, utility::string_t fname)
    {
        fill_file(fname);
        //(withContentLength);
        http_client_config config;
        config.set_chunksize(chunksize);

        test_http_server::scoped_server scoped(address);
        test_http_server* p_server = scoped.server();
        http_client client(address, config);

        p_server->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, methods::POST, U("/"));
            VERIFY_ARE_EQUAL(26u, p_request->m_body.size());
            std::string str_body(std::begin(p_request->m_body), std::end(p_request->m_body));
            VERIFY_ARE_EQUAL(U("abcdefghijklmnopqrstuvwxyz"), ::utility::conversions::to_string_t(str_body));
            p_request->reply(200);
        });

        auto stream = OPEN_R<uint8_t>(fname).get();

        if (withContentLength)
        {
            http_asserts::assert_response_equals(
                client.request(methods::POST, U(""), stream, 26, U("text/plain")).get(), status_codes::OK);
        }
        else
        {
#if defined __cplusplus_winrt
            http_asserts::assert_response_equals(
                client.request(methods::POST, U(""), stream, 26, U("text/plain")).get(), status_codes::OK);
#else
            http_asserts::assert_response_equals(client.request(methods::POST, U(""), stream, U("text/plain")).get(),
                                                 status_codes::OK);
#endif
        }

        stream.close().wait();
    }

#if !defined(__cplusplus_winrt)
    TEST_FIXTURE(uri_address, without_content_length_1)
    {
        stream_request_impl(m_uri, false, 64 * 1024, U("without_content_length_1.txt"));
    }

    TEST_FIXTURE(uri_address, without_content_length_2)
    {
        stream_request_impl(m_uri, false, 1024, U("without_content_length_2.txt"));
    }
#endif

    TEST_FIXTURE(uri_address, with_content_length_1)
    {
        stream_request_impl(m_uri, true, 64 * 1024, U("with_content_length_1.txt"));
    }

    TEST_FIXTURE(uri_address, producer_consumer_buffer_with_content_length)
    {
        streams::producer_consumer_buffer<uint8_t> rbuf;
        fill_buffer(rbuf);
        rbuf.close(std::ios_base::out);

        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();
        http_client client(m_uri);

        http_request msg(methods::POST);
        msg.set_body(streams::istream(rbuf));
        msg.headers().set_content_length(26);

        p_server->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, methods::POST, U("/"));
            VERIFY_ARE_EQUAL(26u, p_request->m_body.size());
            std::string str_body(std::begin(p_request->m_body), std::end(p_request->m_body));
            VERIFY_ARE_EQUAL(U("abcdefghijklmnopqrstuvwxyz"), ::utility::conversions::to_string_t(str_body));
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);
    }

    TEST_FIXTURE(uri_address, stream_partial_from_start)
    {
        utility::string_t fname = U("stream_partial_from_start.txt");
        fill_file(fname, 200);

        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();
        http_client client(m_uri);

        http_request msg(methods::POST);
        auto stream = OPEN_R<uint8_t>(fname).get().create_istream();
        msg.set_body(stream);
        msg.headers().set_content_length(4500);

        p_server->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, methods::POST, U("/"));
            VERIFY_ARE_EQUAL(4500u, p_request->m_body.size());
            std::string str_body(std::begin(p_request->m_body), std::end(p_request->m_body));
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);

        // We should only have read the first 4500 bytes.
        auto length = stream.seek(0, std::ios_base::cur);
        VERIFY_ARE_EQUAL((size_t)length, (size_t)4500);

        stream.close().get();
    }

    TEST_FIXTURE(uri_address, stream_partial_from_middle)
    {
        utility::string_t fname = U("stream_partial_from_middle.txt");
        fill_file(fname, 100);

        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();
        http_client client(m_uri);

        http_request msg(methods::POST);
        auto stream = OPEN_R<uint8_t>(fname).get().create_istream();
        msg.set_body(stream);
        msg.headers().set_content_length(13);

        stream.seek(13, std::ios_base::cur);

        p_server->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, methods::POST, U("/"));
            VERIFY_ARE_EQUAL(13u, p_request->m_body.size());
            std::string str_body(std::begin(p_request->m_body), std::end(p_request->m_body));
            VERIFY_ARE_EQUAL(str_body, "nopqrstuvwxyz");
            p_request->reply(200);
        });
        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);

        // We should only have read the first 26 bytes.
        auto length = stream.seek(0, std::ios_base::cur);
        VERIFY_ARE_EQUAL((int)length, 26);

        stream.close().get();
    }

    class test_exception : public std::exception
    {
    public:
        test_exception() {}
    };

// Ignore on WinRT CodePlex 144
#if !defined(__cplusplus_winrt)
    TEST_FIXTURE(uri_address, set_body_stream_exception)
    {
        test_http_server::scoped_server scoped(m_uri);
        scoped.server();
        http_client client(m_uri);

        streams::producer_consumer_buffer<uint8_t> buf;
        const char* data = "abcdefghijklmnopqrstuvwxyz";
        buf.putn_nocopy(reinterpret_cast<const uint8_t*>(data), 26).wait();

        http_request msg(methods::POST);
        msg.set_body(buf.create_istream());
        msg.headers().set_content_length(26);

        buf.close(std::ios::in, std::make_exception_ptr(test_exception())).wait();

        VERIFY_THROWS(client.request(msg).get(), test_exception);

        // Codeplex 328.
#if !defined(_WIN32)
        tests::common::utilities::os_utilities::sleep(1000);
#endif
    }
#endif

// These tests aren't possible on WinRT because they don't
// specify a Content-Length.
#if !defined(__cplusplus_winrt)
    TEST_FIXTURE(uri_address, stream_close_early)
    {
        http_client client(m_uri);
        test_http_server::scoped_server scoped(m_uri);
        scoped.server()->next_request().then([](test_request* request) { request->reply(status_codes::OK); });

        // Make request.
        streams::producer_consumer_buffer<uint8_t> buf;
        auto responseTask = client.request(methods::PUT, U(""), buf.create_istream());

        // Write a bit of data then close the stream early.
        unsigned char data[5] = {'1', '2', '3', '4', '5'};
        buf.putn_nocopy(&data[0], 5).wait();

        buf.close(std::ios::out).wait();

        // Verify that the task completes successfully
        http_asserts::assert_response_equals(responseTask.get(), status_codes::OK);
    }

    TEST_FIXTURE(uri_address, stream_close_early_with_exception)
    {
        http_client client(m_uri);
        test_http_server::scoped_server scoped(m_uri);

        // Make request.
        streams::producer_consumer_buffer<uint8_t> buf;
        auto responseTask = client.request(methods::PUT, U(""), buf.create_istream());

        // Write a bit of data then close the stream early.
        unsigned char data[5] = {'1', '2', '3', '4', '5'};
        buf.putn_nocopy(&data[0], 5).wait();

        buf.close(std::ios::out, std::make_exception_ptr(test_exception())).wait();

        // Verify that the responseTask throws the exception set when closing the stream
        VERIFY_THROWS(responseTask.get(), test_exception);

        // Codeplex 328.
#if !defined(_WIN32)
        tests::common::utilities::os_utilities::sleep(1000);
#endif
    }
#endif

    // Ignore on WinRT only CodePlex 144
#if !defined(__cplusplus_winrt)
    TEST_FIXTURE(uri_address, stream_close_early_with_exception_and_contentlength)
    {
        http_client client(m_uri);
        test_http_server::scoped_server scoped(m_uri);

        // Make request.
        streams::producer_consumer_buffer<uint8_t> buf;
        auto responseTask = client.request(methods::PUT, U(""), buf.create_istream(), 10);

        // Write a bit of data then close the stream early.
        unsigned char data[5] = {'1', '2', '3', '4', '5'};
        buf.putn_nocopy(&data[0], 5).wait();

        buf.close(std::ios::out, std::make_exception_ptr(test_exception())).wait();

        // Verify that the responseTask throws the exception set when closing the stream
        VERIFY_THROWS(responseTask.get(), test_exception);

        // Codeplex 328.
#if !defined(_WIN32)
        tests::common::utilities::os_utilities::sleep(1000);
#endif
    }
#endif

// Ignore on WinRT only CodePlex 144
#if !defined(__cplusplus_winrt)
    TEST_FIXTURE(uri_address, stream_close_early_with_contentlength, "Ignore:Apple", "328")
    {
        http_client client(m_uri);
        test_http_server::scoped_server scoped(m_uri);

        // Make request.
        streams::producer_consumer_buffer<uint8_t> buf;
        auto responseTask = client.request(methods::PUT, U(""), buf.create_istream(), 10);

        // Write a bit of data then close the stream early.
        unsigned char data[5] = {'1', '2', '3', '4', '5'};
        buf.putn_nocopy(&data[0], 5).wait();

        buf.close(std::ios::out).wait();

        // Verify that the responseTask throws the exception set when closing the stream
        VERIFY_THROWS(responseTask.get(), http_exception);

        // Codeplex 328.
#if !defined(_WIN32)
        tests::common::utilities::os_utilities::sleep(1000);
#endif
    }
#endif

    TEST_FIXTURE(uri_address, get_with_body_nono)
    {
        http_client client(m_uri);

        streams::producer_consumer_buffer<uint8_t> buf;

        http_request reqG(methods::GET);
        reqG.set_body(buf.create_istream());
        VERIFY_THROWS(client.request(reqG).get(), http_exception);

        http_request reqH(methods::HEAD);
        reqH.set_body(buf.create_istream());
        VERIFY_THROWS(client.request(reqH).get(), http_exception);
    }

} // SUITE(request_stream_tests)

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
