/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * response_stream_tests.cpp
 *
 * Tests cases for covering receiving various responses as a stream with http_client.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#if defined(__cplusplus_winrt)
using namespace Windows::Storage;
#endif

#ifndef __cplusplus_winrt
#include "cpprest/http_listener.h"
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
template<typename _CharType>
pplx::task<streams::streambuf<_CharType>> OPENSTR_R(const utility::string_t& name)
{
#if !defined(__cplusplus_winrt)
    return streams::file_buffer<_CharType>::open(name, std::ios_base::in);
#else
    auto file =
        pplx::create_task(KnownFolders::DocumentsLibrary->GetFileAsync(ref new Platform::String(name.c_str()))).get();

    return streams::file_buffer<_CharType>::open(file, std::ios_base::in);
#endif
}

template<typename _CharType>
pplx::task<Concurrency::streams::basic_ostream<_CharType>> OPENSTR_W(const utility::string_t& name,
                                                                     std::ios_base::openmode mode = std::ios_base::out)
{
#if !defined(__cplusplus_winrt)
    return Concurrency::streams::file_stream<_CharType>::open_ostream(name, mode);
#else
    auto file = pplx::create_task(KnownFolders::DocumentsLibrary->CreateFileAsync(
                                      ref new Platform::String(name.c_str()), CreationCollisionOption::ReplaceExisting))
                    .get();

    return Concurrency::streams::file_stream<_CharType>::open_ostream(file, mode);
#endif
}
SUITE(response_stream_tests)
{
    TEST_FIXTURE(uri_address, set_response_stream_producer_consumer_buffer)
    {
        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();
        http_client client(m_uri);

        p_server->next_request().then([&](test_request* p_request) {
            std::map<utility::string_t, utility::string_t> headers;
            headers[U("Content-Type")] = U("text/plain");
            p_request->reply(200, U(""), headers, "This is just a bit of a string");
        });

        streams::producer_consumer_buffer<uint8_t> rwbuf;
        auto ostr = streams::ostream(rwbuf);

        http_request msg(methods::GET);
        msg.set_response_stream(ostr);
        http_response rsp = client.request(msg).get();

        rsp.content_ready().get();
        VERIFY_ARE_EQUAL(rwbuf.in_avail(), 30u);

        VERIFY_THROWS(rsp.extract_string().get(), http_exception);

        char chars[128];
        memset(chars, 0, sizeof(chars));

        rwbuf.getn((unsigned char*)chars, rwbuf.in_avail()).get();
        VERIFY_ARE_EQUAL(0, strcmp("This is just a bit of a string", chars));
    }

    TEST_FIXTURE(uri_address, set_response_stream_container_buffer)
    {
        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();
        http_client client(m_uri);

        p_server->next_request().then([&](test_request* p_request) {
            std::map<utility::string_t, utility::string_t> headers;
            headers[U("Content-Type")] = U("text/plain");
            p_request->reply(200, U(""), headers, "This is just a bit of a string");
        });

        {
            streams::container_buffer<std::vector<uint8_t>> buf;

            http_request msg(methods::GET);
            msg.set_response_stream(buf.create_ostream());
            http_response rsp = client.request(msg).get();

            rsp.content_ready().get();
            VERIFY_ARE_EQUAL(buf.collection().size(), 30);

            char bufStr[31];
            memset(bufStr, 0, sizeof(bufStr));
            memcpy(&bufStr[0], &(buf.collection())[0], 30);
            VERIFY_ARE_EQUAL(bufStr, "This is just a bit of a string");

            VERIFY_THROWS(rsp.extract_string().get(), http_exception);
        }
    }

    TEST_FIXTURE(uri_address, response_stream_file_stream)
    {
        std::string message = "A world without string is chaos.";

        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();
        http_client client(m_uri);

        p_server->next_request().then([&](test_request* p_request) {
            std::map<utility::string_t, utility::string_t> headers;
            headers[U("Content-Type")] = U("text/plain");
            p_request->reply(200, U(""), headers, message);
        });

        {
            auto fstream = OPENSTR_W<uint8_t>(U("response_stream.txt")).get();

            // Write the response into the file
            http_request msg(methods::GET);
            msg.set_response_stream(fstream);
            http_response rsp = client.request(msg).get();

            rsp.content_ready().get();
            VERIFY_IS_TRUE(fstream.streambuf().is_open());
            fstream.close().get();

            char chars[128];
            memset(chars, 0, sizeof(chars));

            streams::rawptr_buffer<uint8_t> buffer(reinterpret_cast<uint8_t*>(chars), sizeof(chars));

            streams::basic_istream<uint8_t> fistream = OPENSTR_R<uint8_t>(U("response_stream.txt")).get();
            VERIFY_ARE_EQUAL(message.length(), fistream.read_line(buffer).get());
            VERIFY_ARE_EQUAL(message, std::string(chars));
            fistream.close().get();
        }
    }

    TEST_FIXTURE(uri_address, response_stream_file_stream_close_early)
    {
        // The test needs to be a little different between desktop and WinRT.
        // In the latter case, the server will not see a message, and so the
        // test will hang. In order to prevent that from happening, we will
        // not have a server listening on WinRT.
#if !defined(__cplusplus_winrt)
        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();

        p_server->next_request().then([&](test_request* p_request) {
            std::map<utility::string_t, utility::string_t> headers;
            headers[U("Content-Type")] = U("text/plain");
            p_request->reply(200, U(""), headers, "A world without string is chaos.");
        });
#endif

        auto fstream = OPENSTR_W<uint8_t>(U("response_stream_file_stream_close_early.txt")).get();

        http_client client(m_uri);

        http_request msg(methods::GET);
        msg.set_response_stream(fstream);
        fstream.close(std::make_exception_ptr(std::exception())).wait();

        http_response resp;

        VERIFY_THROWS((resp = client.request(msg).get(), resp.content_ready().get()), std::exception);
    }

    TEST_FIXTURE(uri_address, response_stream_large_file_stream)
    {
        // Send a 100 KB data in the response body, the server will send this in multiple chunks
        // This data will get sent with content-length
        const size_t workload_size = 100 * 1024;
        utility::string_t fname(U("response_stream_large_file_stream.txt"));
        std::string responseData;
        responseData.resize(workload_size, 'a');

        test_http_server::scoped_server scoped(m_uri);
        test_http_server* p_server = scoped.server();

        http_client client(m_uri);

        p_server->next_request().then([&](test_request* p_request) {
            std::map<utility::string_t, utility::string_t> headers;
            headers[U("Content-Type")] = U("text/plain");

            p_request->reply(200, U(""), headers, responseData);
        });

        {
            auto fstream = OPENSTR_W<uint8_t>(fname).get();

            http_request msg(methods::GET);
            msg.set_response_stream(fstream);
            http_response rsp = client.request(msg).get();

            rsp.content_ready().get();
            VERIFY_IS_TRUE(fstream.streambuf().is_open());
            fstream.close().get();

            std::string rsp_string;
            rsp_string.resize(workload_size, 0);
            streams::rawptr_buffer<char> buffer(&rsp_string[0], rsp_string.size());
            streams::basic_istream<char> fistream = OPENSTR_R<char>(fname).get();

            VERIFY_ARE_EQUAL(fistream.read_to_end(buffer).get(), workload_size);
            VERIFY_ARE_EQUAL(rsp_string, responseData);
            fistream.close().get();
        }
    }

#if !defined(__cplusplus_winrt)

    template<typename CharType>
    class basic_throws_buffer : public streams::details::streambuf_state_manager<CharType>
    {
    public:
        basic_throws_buffer() : streams::details::streambuf_state_manager<CharType>(std::ios_base::out) {}

        typedef typename streams::details::basic_streambuf<CharType>::int_type int_type;
        typedef typename streams::details::basic_streambuf<CharType>::pos_type pos_type;
        typedef typename streams::details::basic_streambuf<CharType>::off_type off_type;

        bool can_seek() const override { return true; }
        bool has_size() const override { return false; }
        size_t buffer_size(std::ios_base::openmode) const override { return 0; }
        void set_buffer_size(size_t, std::ios_base::openmode) override {}
        size_t in_avail() const override { return 0; }
        pos_type getpos(std::ios_base::openmode) const override { return 0; }
        pos_type seekpos(pos_type, std::ios_base::openmode) override { return 0; }
        pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode) override { return 0; }
        bool acquire(_Out_writes_(count) CharType*&, _In_ size_t&) override { return false; }
        void release(_Out_writes_(count) CharType*, _In_ size_t) override {}

    protected:
        pplx::task<int_type> _putc(CharType) override { throw std::runtime_error("error"); }
        pplx::task<size_t> _putn(const CharType*, size_t) override { throw std::runtime_error("error"); }
        pplx::task<int_type> _bumpc() override { throw std::runtime_error("error"); }
        int_type _sbumpc() override { throw std::runtime_error("error"); }
        pplx::task<int_type> _getc() override { throw std::runtime_error("error"); }
        int_type _sgetc() override { throw std::runtime_error("error"); }
        pplx::task<int_type> _nextc() override { throw std::runtime_error("error"); }
        pplx::task<int_type> _ungetc() override { throw std::runtime_error("error"); }
        pplx::task<size_t> _getn(_Out_writes_(count) CharType*, _In_ size_t) override
        {
            throw std::runtime_error("error");
        }
        size_t _scopy(_Out_writes_(count) CharType*, _In_ size_t) override { throw std::runtime_error("error"); }
        pplx::task<bool> _sync() override { throw std::runtime_error("error"); }
        CharType* _alloc(size_t) override { throw std::runtime_error("error"); }
        void _commit(size_t) override { throw std::runtime_error("error"); }

        pplx::task<void> _close_write() override
        {
            return pplx::task_from_exception<void>(std::invalid_argument("test"));
        }
    };

    template<typename CharType>
    class close_throws_buffer : public streams::streambuf<CharType>
    {
    public:
        close_throws_buffer()
            : streams::streambuf<CharType>(
                  std::shared_ptr<basic_throws_buffer<CharType>>(new basic_throws_buffer<CharType>()))
        {
        }
    };

    // Tests if an exception occurs and close throws an exception that the close
    // one is ignored and doesn't bring down the process.
    TEST_FIXTURE(uri_address, response_stream_close_throws_with_exception)
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

        http_client_config config;
        config.set_timeout(utility::seconds(1));
        http_client client(m_uri, config);

        close_throws_buffer<uint8_t> responseBody;
        http_request msg(methods::GET);
        msg.set_response_stream(responseBody.create_ostream());
        http_response rsp = client.request(msg).get();
        VERIFY_THROWS(rsp.content_ready().get(), http_exception);

        buf.close(std::ios_base::out).wait();
        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, content_ready)
    {
        http_client client(m_uri);
        std::string responseData("Hello world");

        web::http::experimental::listener::http_listener listener(m_uri);
        listener.open().wait();
        listener.support([responseData](http_request request) {
            streams::producer_consumer_buffer<uint8_t> buf;
            http_response response(200);
            response.set_body(buf.create_istream(), U("text/plain"));
            response.headers().add(header_names::connection, U("close"));

            request.reply(response);

            VERIFY_ARE_EQUAL(buf.putn_nocopy((const uint8_t*)responseData.data(), responseData.size()).get(),
                             responseData.size());
            buf.close(std::ios_base::out).get();
        });

        {
            http_request msg(methods::GET);
            http_response rsp = client.request(msg).get().content_ready().get();

            auto extract_string_task = rsp.extract_string();
            VERIFY_ARE_EQUAL(extract_string_task.get(), ::utility::conversions::to_string_t(responseData));
            rsp.content_ready().wait();
        }

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, xfer_chunked_with_length)
    {
        http_client client(m_uri);
        utility::string_t responseData(U("Hello world"));

        web::http::experimental::listener::http_listener listener(m_uri);
        listener.open().wait();
        listener.support([responseData](http_request request) {
            http_response response(200);

            // This sets the content_length
            response.set_body(responseData);

            // overwrite content_length to 0
            response.headers().add(header_names::content_length, 0);

            // add chunked transfer encoding
            response.headers().add(header_names::transfer_encoding, U("chunked"));

            // add connection=close header, connection SHOULD NOT be considered persistent' after the current
            // request/response is complete
            response.headers().add(header_names::connection, U("close"));

            // respond
            request.reply(response);
        });

        {
            http_request msg(methods::GET);
            http_response rsp = client.request(msg).get();

            auto rsp_string = rsp.extract_string().get();
            VERIFY_ARE_EQUAL(rsp_string, responseData);
        }

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, get_resp_stream)
    {
        http_client client(m_uri);
        std::string responseData("Hello world");

        web::http::experimental::listener::http_listener listener(m_uri);
        listener.open().wait();
        listener.support([responseData](http_request request) {
            streams::producer_consumer_buffer<uint8_t> buf;

            http_response response(200);
            response.set_body(buf.create_istream(), U("text/plain"));
            response.headers().add(header_names::connection, U("close"));
            request.reply(response);

            VERIFY_ARE_EQUAL(buf.putn_nocopy((const uint8_t*)responseData.data(), responseData.size()).get(),
                             responseData.size());
            buf.close(std::ios_base::out).get();
        });

        {
            http_request msg(methods::GET);
            http_response rsp = client.request(msg).get();

            streams::stringstreambuf data;

            auto t = rsp.body().read_to_delim(data, (uint8_t)(' '));

            t.then([&data](size_t size) {
                 VERIFY_ARE_EQUAL(size, 5);
                 auto s = data.collection();
                 VERIFY_ARE_EQUAL(s, std::string("Hello"));
             })
                .wait();
            rsp.content_ready().wait();
        }

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, xfer_chunked_multiple_chunks)
    {
        // With chunked transfer-encoding, send 2 chunks of different sizes in the response
        http_client client(m_uri);

        // Send two chunks, note: second chunk is bigger than the first.
        std::string firstChunk("abcdefghijklmnopqrst");
        std::string secondChunk("abcdefghijklmnopqrstuvwxyz");

        web::http::experimental::listener::http_listener listener(m_uri);
        listener.open().wait();
        listener.support([firstChunk, secondChunk](http_request request) {
            streams::producer_consumer_buffer<uint8_t> buf;

            http_response response(200);
            response.set_body(buf.create_istream(), U("text/plain"));
            response.headers().add(header_names::connection, U("close"));
            request.reply(response);

            VERIFY_ARE_EQUAL(buf.putn_nocopy((const uint8_t*)firstChunk.data(), firstChunk.size()).get(),
                             firstChunk.size());
            buf.sync().get();
            VERIFY_ARE_EQUAL(buf.putn_nocopy((const uint8_t*)secondChunk.data(), secondChunk.size()).get(),
                             secondChunk.size());
            buf.close(std::ios_base::out).get();
        });

        {
            utility::string_t fname(U("xfer_chunked_multiple_chunks.txt"));
            auto fstream = OPENSTR_W<uint8_t>(fname).get();

            http_request msg(methods::GET);
            msg.set_response_stream(fstream);
            http_response rsp = client.request(msg).get();

            rsp.content_ready().wait();
            VERIFY_IS_TRUE(fstream.streambuf().is_open());
            fstream.close().get();

            std::string rsp_string;
            size_t workload_size = firstChunk.size() + secondChunk.size();
            rsp_string.resize(workload_size, 0);
            streams::rawptr_buffer<uint8_t> buffer(reinterpret_cast<uint8_t*>(&rsp_string[0]), rsp_string.size());
            streams::basic_istream<uint8_t> fistream = OPENSTR_R<uint8_t>(fname).get();

            VERIFY_ARE_EQUAL(fistream.read_to_end(buffer).get(), workload_size);
            VERIFY_ARE_EQUAL(rsp_string, firstChunk + secondChunk);
            fistream.close().get();
        }

        listener.close().wait();
    }

#endif

} // SUITE(responses)

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
