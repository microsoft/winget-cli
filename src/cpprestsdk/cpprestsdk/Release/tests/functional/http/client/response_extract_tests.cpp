/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * response_extract_tests.cpp
 *
 * Tests cases covering extract functions on HTTP response.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#ifndef __cplusplus_winrt
#include "cpprest/http_listener.h"
#endif

using namespace web;
using namespace utility;
using namespace concurrency;
using namespace utility::conversions;
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
SUITE(response_extract_tests)
{
    // Helper function to send a request and response with given values.
    template<typename CharType>
    static http_response send_request_response(test_http_server * p_server,
                                               http_client * p_client,
                                               const utility::string_t& content_type,
                                               const std::basic_string<CharType>& data)
    {
        const method method = methods::GET;
        const ::http::status_code code = status_codes::OK;
        std::map<utility::string_t, utility::string_t> headers;
        if (!content_type.empty())
        {
            headers[U("Content-Type")] = content_type;
        }
        p_server->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, method, U("/"));
            VERIFY_ARE_EQUAL(0u, p_request->reply(code, U(""), headers, data));
        });
        http_response rsp = p_client->request(method).get();
        http_asserts::assert_response_equals(rsp, code, headers);
        return rsp;
    }

    utf16string switch_endian_ness(const utf16string& src_str)
    {
        utf16string dest_str;
        dest_str.resize(src_str.size());
        unsigned char* src = (unsigned char*)&src_str[0];
        unsigned char* dest = (unsigned char*)&dest_str[0];
        for (size_t i = 0; i < dest_str.size() * 2; i += 2)
        {
            dest[i] = src[i + 1];
            dest[i + 1] = src[i];
        }
        return dest_str;
    }

    TEST_FIXTURE(uri_address, extract_string)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        // default encoding (Latin1)
        std::string data("YOU KNOW ITITITITI");
        http_response rsp = send_request_response(scoped.server(), &client, U("text/plain"), data);
        VERIFY_ARE_EQUAL(to_string_t(data), rsp.extract_string().get());

        // us-ascii
        rsp = send_request_response(scoped.server(), &client, U("text/plain;  charset=  us-AscIi"), data);
        VERIFY_ARE_EQUAL(to_string_t(data), rsp.extract_string().get());

        // Latin1
        rsp = send_request_response(scoped.server(), &client, U("text/plain;charset=iso-8859-1"), data);
        VERIFY_ARE_EQUAL(to_string_t(data), rsp.extract_string().get());

        // utf-8
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset  =  UTF-8"), data);
        VERIFY_ARE_EQUAL(to_string_t(data), rsp.extract_string().get());

        // "utf-8" - quoted charset
        rsp = send_request_response(scoped.server(), &client, U("text/plain;charset=\"utf-8\""), data);
        VERIFY_ARE_EQUAL(to_string_t(data), rsp.extract_string().get());

        // no content length
        rsp = send_request_response(scoped.server(), &client, U(""), utility::string_t());
        auto str = rsp.to_string();
        // If there is no Content-Type in the response, make sure it won't throw when we ask for string
        if (str.find(U("Content-Type")) == std::string::npos)
        {
            VERIFY_ARE_EQUAL(utility::string_t(U("")), rsp.extract_string().get());
        }

        // utf-16le
        data = "YES NOW, HERHEHE****";
        utf16string wdata(utf8_to_utf16(data));
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=utf-16le"), wdata);
        VERIFY_ARE_EQUAL(to_string_t(data), rsp.extract_string().get());

        // utf-16be
        wdata = switch_endian_ness(wdata);
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=utf-16be"), wdata);
        VERIFY_ARE_EQUAL(to_string_t(data), rsp.extract_string().get());

        // utf-16 no BOM (utf-16be)
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=utf-16"), wdata);
        VERIFY_ARE_EQUAL(to_string_t(data), rsp.extract_string().get());

        // utf-16 big endian BOM.
        wdata.insert(wdata.begin(), ('\0'));
        unsigned char* start = (unsigned char*)&wdata[0];
        start[0] = 0xFE;
        start[1] = 0xFF;
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=utf-16"), wdata);
        VERIFY_ARE_EQUAL(to_string_t(data), rsp.extract_string().get());

        // utf-16 little endian BOM.
        wdata = utf8_to_utf16("YOU KNOW THIS **********KICKS");
        data = utf16_to_utf8(wdata);
        wdata.insert(wdata.begin(), '\0');
        start = (unsigned char*)&wdata[0];
        start[0] = 0xFF;
        start[1] = 0xFE;
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=utf-16"), wdata);
        VERIFY_ARE_EQUAL(to_string_t(data), rsp.extract_string().get());
    }

    TEST_FIXTURE(uri_address, extract_utf8string)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        // default encoding (Latin1)
        std::string data("YOU KNOW ITITITITI");
        http_response rsp = send_request_response(scoped.server(), &client, U("text/plain"), data);
        VERIFY_ARE_EQUAL(data, rsp.extract_utf8string().get());

        // us-ascii
        rsp = send_request_response(scoped.server(), &client, U("text/plain;  charset=  us-AscIi"), data);
        VERIFY_ARE_EQUAL(data, rsp.extract_utf8string().get());

        // Latin1
        rsp = send_request_response(scoped.server(), &client, U("text/plain;charset=iso-8859-1"), data);
        VERIFY_ARE_EQUAL(data, rsp.extract_utf8string().get());

        // utf-8
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset  =  UTF-8"), data);
        VERIFY_ARE_EQUAL(data, rsp.extract_utf8string().get());

        // "utf-8" - quoted charset
        rsp = send_request_response(scoped.server(), &client, U("text/plain;charset=\"utf-8\""), data);
        VERIFY_ARE_EQUAL(data, rsp.extract_utf8string().get());

        // no content length
        rsp = send_request_response(scoped.server(), &client, U(""), utility::string_t());
        auto str = rsp.to_string();
        // If there is no Content-Type in the response, make sure it won't throw when we ask for string
        if (str.find(U("Content-Type")) == std::string::npos)
        {
            VERIFY_ARE_EQUAL("", rsp.extract_utf8string().get());
        }

        // utf-16le
        data = "YES NOW, HERHEHE****";
        utf16string wdata(utf8_to_utf16(data));
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=utf-16le"), wdata);
        VERIFY_ARE_EQUAL(data, rsp.extract_utf8string().get());

        // utf-16be
        wdata = switch_endian_ness(wdata);
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=utf-16be"), wdata);
        VERIFY_ARE_EQUAL(data, rsp.extract_utf8string().get());

        // utf-16 no BOM (utf-16be)
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=utf-16"), wdata);
        VERIFY_ARE_EQUAL(data, rsp.extract_utf8string().get());

        // utf-16 big endian BOM.
        wdata.insert(wdata.begin(), ('\0'));
        unsigned char* start = (unsigned char*)&wdata[0];
        start[0] = 0xFE;
        start[1] = 0xFF;
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=utf-16"), wdata);
        VERIFY_ARE_EQUAL(data, rsp.extract_utf8string().get());

        // utf-16 little endian BOM.
        wdata = utf8_to_utf16("YOU KNOW THIS **********KICKS");
        data = utf16_to_utf8(wdata);
        wdata.insert(wdata.begin(), '\0');
        start = (unsigned char*)&wdata[0];
        start[0] = 0xFF;
        start[1] = 0xFE;
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=utf-16"), wdata);
        VERIFY_ARE_EQUAL(data, rsp.extract_utf8string().get());
    }

    TEST_FIXTURE(uri_address, extract_utf16string)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        // default encoding (Latin1)
        std::string data("YOU KNOW ITITITITI");
        utf16string wdata(utf8_to_utf16(data));
        http_response rsp = send_request_response(scoped.server(), &client, U("text/plain"), data);
        VERIFY_ARE_EQUAL(wdata, rsp.extract_utf16string().get());

        // us-ascii
        rsp = send_request_response(scoped.server(), &client, U("text/plain;  charset=  us-AscIi"), data);
        VERIFY_ARE_EQUAL(wdata, rsp.extract_utf16string().get());

        // Latin1
        rsp = send_request_response(scoped.server(), &client, U("text/plain;charset=iso-8859-1"), data);
        VERIFY_ARE_EQUAL(wdata, rsp.extract_utf16string().get());

        // utf-8
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset  =  UTF-8"), data);
        VERIFY_ARE_EQUAL(wdata, rsp.extract_utf16string().get());

        // "utf-8" - quoted charset
        rsp = send_request_response(scoped.server(), &client, U("text/plain;charset=\"utf-8\""), data);
        VERIFY_ARE_EQUAL(wdata, rsp.extract_utf16string().get());

        // no content length
        rsp = send_request_response(scoped.server(), &client, U(""), utility::string_t());
        auto str = rsp.to_string();
        // If there is no Content-Type in the response, make sure it won't throw when we ask for string
        if (str.find(U("Content-Type")) == std::string::npos)
        {
            VERIFY_ARE_EQUAL(utf16string(), rsp.extract_utf16string().get());
        }

        // utf-16le
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=utf-16le"), wdata);
        VERIFY_ARE_EQUAL(wdata, rsp.extract_utf16string().get());

        // utf-16be
        auto wdatabe = switch_endian_ness(wdata);
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=utf-16be"), wdatabe);
        VERIFY_ARE_EQUAL(wdata, rsp.extract_utf16string().get());

        // utf-16 no BOM (utf-16be)
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=utf-16"), wdatabe);
        VERIFY_ARE_EQUAL(wdata, rsp.extract_utf16string().get());

        // utf-16 big endian BOM.
        wdatabe.insert(wdatabe.begin(), ('\0'));
        unsigned char* start = (unsigned char*)&wdatabe[0];
        start[0] = 0xFE;
        start[1] = 0xFF;
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=utf-16"), wdatabe);
        VERIFY_ARE_EQUAL(wdata, rsp.extract_utf16string().get());

        // utf-16 little endian BOM.
        auto wdatale = wdata;
        wdatale.insert(wdatale.begin(), '\0');
        start = (unsigned char*)&wdatale[0];
        start[0] = 0xFF;
        start[1] = 0xFE;
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=utf-16"), wdatale);
        VERIFY_ARE_EQUAL(wdata, rsp.extract_utf16string().get());
    }

    TEST_FIXTURE(uri_address, extract_string_force)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        std::string data("YOU KNOW ITITITITI");
        http_response rsp = send_request_response(scoped.server(), &client, U("bad unknown charset"), data);
        VERIFY_ARE_EQUAL(to_string_t(data), rsp.extract_string(true).get());
        rsp = send_request_response(scoped.server(), &client, U("bad unknown charset"), data);
        VERIFY_ARE_EQUAL(data, rsp.extract_utf8string(true).get());
        rsp = send_request_response(scoped.server(), &client, U("bad unknown charset"), data);
        VERIFY_ARE_EQUAL(to_utf16string(data), rsp.extract_utf16string(true).get());
    }

    TEST_FIXTURE(uri_address, extract_string_incorrect)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        // with non matching content type.
        const std::string data("YOU KNOW ITITITITI");
        http_response rsp = send_request_response(scoped.server(), &client, U("non_text"), data);
        VERIFY_THROWS(rsp.extract_string().get(), http_exception);

        // with unknown charset
        rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=uis-ascii"), data);
        VERIFY_THROWS(rsp.extract_string().get(), http_exception);
    }

#ifndef __cplusplus_winrt
    TEST_FIXTURE(uri_address, extract_empty_string)
    {
        web::http::experimental::listener::http_listener listener(m_uri);
        http_client client(m_uri);
        listener.support([](http_request msg) {
            auto ResponseStreamBuf = streams::producer_consumer_buffer<uint8_t>();
            ResponseStreamBuf.close(std::ios_base::out).wait();
            http_response response(status_codes::OK);
            response.set_body(ResponseStreamBuf.create_istream(), U("text/plain"));
            response.headers().add(header_names::connection, U("close"));
            msg.reply(response).wait();
        });

        listener.open().wait();

        auto response = client.request(methods::GET).get();
        auto data = response.extract_string().get();

        VERIFY_ARE_EQUAL(0, data.size());
        listener.close().wait();
    }
#endif

    TEST_FIXTURE(uri_address, extract_json)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        // default encoding (Latin1)
        json::value data = json::value::string(U("JSON string object"));
        http_response rsp =
            send_request_response(scoped.server(), &client, U("application/json"), to_utf8string(data.serialize()));
        VERIFY_ARE_EQUAL(data.serialize(), rsp.extract_json().get().serialize());

        // us-ascii
        rsp = send_request_response(
            scoped.server(), &client, U("application/json;  charset=  us-AscIi"), to_utf8string(data.serialize()));
        VERIFY_ARE_EQUAL(data.serialize(), rsp.extract_json().get().serialize());

        // Latin1
        rsp = send_request_response(
            scoped.server(), &client, U("application/json;charset=iso-8859-1"), to_utf8string(data.serialize()));
        VERIFY_ARE_EQUAL(data.serialize(), rsp.extract_json().get().serialize());

        // utf-8
        rsp = send_request_response(
            scoped.server(), &client, U("application/json; charset  =  UTF-8"), to_utf8string((data.serialize())));
        VERIFY_ARE_EQUAL(data.serialize(), rsp.extract_json().get().serialize());

        rsp = send_request_response(scoped.server(), &client, U(""), utility::string_t());
        auto str = rsp.to_string();
        // If there is no Content-Type in the response, make sure it won't throw when we ask for json
        if (str.find(U("Content-Type")) == std::string::npos)
        {
            VERIFY_ARE_EQUAL(utility::string_t(U("null")), rsp.extract_json().get().serialize());
        }

#ifdef _WIN32
        // utf-16le
        auto utf16str = data.serialize();
        rsp = send_request_response(scoped.server(), &client, U("application/json; charset=utf-16le"), utf16str);
        VERIFY_ARE_EQUAL(data.serialize(), rsp.extract_json().get().serialize());

        // utf-16be
        utf16string modified_data = data.serialize();
        modified_data = switch_endian_ness(modified_data);
        rsp = send_request_response(scoped.server(), &client, U("application/json; charset=utf-16be"), modified_data);
        VERIFY_ARE_EQUAL(data.serialize(), rsp.extract_json().get().serialize());

        // utf-16 no BOM (utf-16be)
        rsp = send_request_response(scoped.server(), &client, U("application/json; charset=utf-16"), modified_data);
        VERIFY_ARE_EQUAL(data.serialize(), rsp.extract_json().get().serialize());

        // utf-16 big endian BOM.
        modified_data.insert(modified_data.begin(), U('\0'));
        unsigned char* start = (unsigned char*)&modified_data[0];
        start[0] = 0xFE;
        start[1] = 0xFF;
        rsp = send_request_response(scoped.server(), &client, U("application/json; charset=utf-16"), modified_data);
        VERIFY_ARE_EQUAL(data.serialize(), rsp.extract_json().get().serialize());

        // utf-16 little endian BOM.
        modified_data = data.serialize();
        modified_data.insert(modified_data.begin(), U('\0'));
        start = (unsigned char*)&modified_data[0];
        start[0] = 0xFF;
        start[1] = 0xFE;
        rsp = send_request_response(scoped.server(), &client, U("application/json; charset=utf-16"), modified_data);
        VERIFY_ARE_EQUAL(data.serialize(), rsp.extract_json().get().serialize());
#endif

        // unofficial JSON MIME types
        rsp = send_request_response(scoped.server(), &client, U("text/json"), to_utf8string(data.serialize()));
        VERIFY_ARE_EQUAL(data.serialize(), rsp.extract_json().get().serialize());
        rsp = send_request_response(scoped.server(), &client, U("text/x-json"), to_utf8string(data.serialize()));
        VERIFY_ARE_EQUAL(data.serialize(), rsp.extract_json().get().serialize());
        rsp = send_request_response(scoped.server(), &client, U("text/javascript"), to_utf8string(data.serialize()));
        VERIFY_ARE_EQUAL(data.serialize(), rsp.extract_json().get().serialize());
        rsp = send_request_response(scoped.server(), &client, U("text/x-javascript"), to_utf8string(data.serialize()));
        VERIFY_ARE_EQUAL(data.serialize(), rsp.extract_json().get().serialize());
        rsp = send_request_response(
            scoped.server(), &client, U("application/javascript"), to_utf8string(data.serialize()));
        VERIFY_ARE_EQUAL(data.serialize(), rsp.extract_json().get().serialize());
        rsp = send_request_response(
            scoped.server(), &client, U("application/x-javascript"), to_utf8string(data.serialize()));
        VERIFY_ARE_EQUAL(data.serialize(), rsp.extract_json().get().serialize());
    }

    TEST_FIXTURE(uri_address, extract_json_force)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        json::value data = json::value::string(U("JSON string object"));
        http_response rsp =
            send_request_response(scoped.server(), &client, U("bad charset"), to_utf8string(data.serialize()));
        VERIFY_ARE_EQUAL(data.serialize(), rsp.extract_json(true).get().serialize());
    }

    TEST_FIXTURE(uri_address, extract_json_incorrect)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        // with non matching content type.
        json::value json_data = json::value::string(U("JSON string object"));
        http_response rsp = send_request_response(scoped.server(), &client, U("bad guy"), json_data.serialize());
        VERIFY_THROWS(rsp.extract_json().get(), http_exception);

        // with unknown charset.
        rsp = send_request_response(
            scoped.server(), &client, U("application/json; charset=us-askjhcii"), json_data.serialize());
        VERIFY_THROWS(rsp.extract_json().get(), http_exception);
    }

    TEST_FIXTURE(uri_address, set_stream_try_extract_json)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        http_request request(methods::GET);
        streams::ostream responseStream = streams::bytestream::open_ostream<std::vector<uint8_t>>();
        request.set_response_stream(responseStream);
        scoped.server()->next_request().then([](test_request* req) {
            std::map<utility::string_t, utility::string_t> headers;
            headers[header_names::content_type] = U("application/json");
            req->reply(status_codes::OK, U("OK"), headers, U("{true}"));
        });

        http_response response = client.request(request).get();
        VERIFY_THROWS(response.extract_json().get(), http_exception);
    }

    TEST_FIXTURE(uri_address, extract_vector)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        // textual content type - with unknown charset
        std::string data("YOU KNOW ITITITITI");
        std::vector<unsigned char> vector_data;
        std::for_each(data.begin(), data.end(), [&](char ch) { vector_data.push_back((unsigned char)ch); });
        http_response rsp = send_request_response(scoped.server(), &client, U("text/plain; charset=unknown"), data);
        VERIFY_ARE_EQUAL(vector_data, rsp.extract_vector().get());

        // textual type with us-ascii
        rsp = send_request_response(scoped.server(), &client, U("text/plain;  charset=  us-AscIi"), data);
        VERIFY_ARE_EQUAL(vector_data, rsp.extract_vector().get());

        // textual type with Latin1
        rsp = send_request_response(scoped.server(), &client, U("text/plain;  charset=iso-8859-1"), data);
        VERIFY_ARE_EQUAL(vector_data, rsp.extract_vector().get());

        // textual type with utf-8
        rsp = send_request_response(scoped.server(), &client, U("text/plain;  charset=utf-8"), data);
        VERIFY_ARE_EQUAL(vector_data, rsp.extract_vector().get());

        // textual type with utf-16le
        rsp = send_request_response(scoped.server(), &client, U("text/plain;  charset=utf-16LE"), data);
        VERIFY_ARE_EQUAL(vector_data, rsp.extract_vector().get());

        // textual type with utf-16be
        rsp = send_request_response(scoped.server(), &client, U("text/plain;  charset=UTF-16be"), data);
        VERIFY_ARE_EQUAL(vector_data, rsp.extract_vector().get());

        // textual type with utf-16
        rsp = send_request_response(scoped.server(), &client, U("text/plain;  charset=utf-16"), data);
        VERIFY_ARE_EQUAL(vector_data, rsp.extract_vector().get());

        // non textual content type
        rsp = send_request_response(scoped.server(), &client, U("blah;  charset=utf-16"), data);
        VERIFY_ARE_EQUAL(vector_data, rsp.extract_vector().get());
    }

    TEST_FIXTURE(uri_address, set_stream_try_extract_vector)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        http_request request(methods::GET);
        streams::ostream responseStream = streams::bytestream::open_ostream<std::vector<uint8_t>>();
        request.set_response_stream(responseStream);
        scoped.server()->next_request().then([](test_request* req) {
            std::map<utility::string_t, utility::string_t> headers;
            headers[header_names::content_type] = U("text/plain");
            req->reply(status_codes::OK, U("OK"), headers, U("data"));
        });

        http_response response = client.request(request).get();
        VERIFY_THROWS(response.extract_vector().get(), http_exception);
    }

    TEST_FIXTURE(uri_address, head_response)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        const method method = methods::HEAD;
        const ::http::status_code code = status_codes::OK;
        std::map<utility::string_t, utility::string_t> headers;
        headers[U("Content-Type")] = U("text/plain");
        headers[U("Content-Length")] = U("100");
        scoped.server()->next_request().then([&](test_request* p_request) {
            http_asserts::assert_test_request_equals(p_request, method, U("/"));
            VERIFY_ARE_EQUAL(0u, p_request->reply(code, U(""), headers));
        });
        http_response rsp = client.request(method).get();
        VERIFY_ARE_EQUAL(0u, rsp.body().streambuf().in_avail());
    }

} // SUITE(response_extract_tests)

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
