/***
 * ==++==
 *
 * Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 * ==--==
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * requests_tests.cpp
 *
 * Tests cases for covering sending various requests to http_listener.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include <cpprest/http_client.h>

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
SUITE(requests_tests)
{
    TEST_FIXTURE(uri_address, http_methods)
    {
        http_listener listener(m_uri);

        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        // Don't include 'CONNECT' it has a special meaning.
        utility::string_t send_methods[] = {methods::GET,
                                            U("GET"),
                                            methods::DEL,
                                            methods::HEAD,
                                            U("HeAd"),
                                            methods::POST,
                                            methods::PUT,
                                            U("CUstomMETHOD")};
        utility::string_t recv_methods[] = {
            U("GET"), U("GET"), U("DELETE"), U("HEAD"), U("HEAD"), U("POST"), U("PUT"), U("CUstomMETHOD")};
        const size_t num_methods = sizeof(send_methods) / sizeof(send_methods[0]);

        utility::string_t actual_method;
        listener.support([&](http_request request) {
            actual_method = request.method();
            request.reply(status_codes::OK).wait();
        });

        for (int i = 0; i < num_methods; ++i)
        {
            pplx::extensibility::event_t ev;
            VERIFY_ARE_EQUAL(0, p_client->request(send_methods[i], U("")));
            p_client->next_response()
                .then([&ev](test_response* p_response) {
                    http_asserts::assert_test_response_equals(p_response, status_codes::OK);
                    ev.set();
                })
                .wait();
            VERIFY_ARE_EQUAL(recv_methods[i], actual_method);
            ev.wait();
        }

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, http_body_and_body_size)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        // request with no body
        listener.support([](http_request request) {
            http_asserts::assert_request_equals(request, U("GET"), U("/"));
            VERIFY_ARE_EQUAL(0, request.body().streambuf().in_avail());
            request.reply(status_codes::OK);
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        // request with body size explicitly 0
        listener.support([](http_request request) {
            http_asserts::assert_request_equals(request, U("GET"), U("/"));
            VERIFY_ARE_EQUAL(0, request.body().streambuf().in_avail());
            request.reply(status_codes::OK);
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U(""), ""));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        // request with body data
        std::string data("HEHE");
        listener.support([&](http_request request) {
            http_asserts::assert_request_equals(request, U("GET"), U("/"));

            auto stream = request.body();
            VERIFY_IS_TRUE(stream.is_valid());
            auto buf = stream.streambuf();
            VERIFY_IS_TRUE(buf);

            request.content_ready().wait();

            VERIFY_ARE_EQUAL(data.size(), buf.in_avail());
            VERIFY_ARE_EQUAL('H', (char)buf.sbumpc());
            VERIFY_ARE_EQUAL('E', (char)buf.sbumpc());
            VERIFY_ARE_EQUAL('H', (char)buf.sbumpc());
            VERIFY_ARE_EQUAL('E', (char)buf.sbumpc());
            request.reply(status_codes::OK);
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U(""), data));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        listener.close().wait();
    }

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
        listener.support([&](http_request request) {
            std::string recv_data = utility::conversions::to_utf8string(request.extract_string().get());
            VERIFY_ARE_EQUAL(send_data, recv_data);
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

    TEST_FIXTURE(uri_address, response_order)
    {
        http_listener listener(m_uri);
        listener.open().wait();

        client::http_client_config config;
        // our product client would be able to pipe multiple requests on one connection
        client::http_client client(m_uri, config);

        const int num_requests = 50;

        listener.support([](http_request request) {
            auto str = request.extract_string().get();
            // intentionally break order
            if (str == U("0")) tests::common::utilities::os_utilities::sleep(500);
            request.reply(status_codes::OK, str);
        });

        std::vector<pplx::task<web::http::http_response>> responses;

        for (int i = 0; i < num_requests; ++i)
        {
            utility::ostringstream_t ss;
            ss << i;
            responses.push_back(client.request(web::http::methods::PUT, U(""), ss.str()));
        }

        // wait for requests.
        for (size_t i = 0; i < num_requests; ++i)
        {
            utility::ostringstream_t ss;
            ss << i;
            auto response = responses[i].get();

            // verify the requests and responses are still match
            VERIFY_ARE_EQUAL(response.status_code(), status_codes::OK);
            VERIFY_ARE_EQUAL(response.extract_string().get(), ss.str());
        }

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, uri_encoding, "Ignore", "Codeplex 201")
    {
        http_listener listener(m_uri);
        listener.open().wait();
        client::http_client client(m_uri);
        utility::string_t encoded_uri;

        listener.support([&](http_request request) {
            VERIFY_ARE_EQUAL(encoded_uri, request.relative_uri().to_string());
            request.reply(status_codes::OK);
        });

        // Wrap in try catch to print out more information to help with a sporadic failure.
        try
        {
            encoded_uri = uri::encode_uri(U("/path 1/path 2")); // Path component contains encoded characters
            client.request(methods::GET, encoded_uri).wait();
            encoded_uri = uri::encode_uri(
                U("/test?Text=J'ai besoin de trouver un personnage")); // Query string contains encoded characters
            client.request(methods::GET, encoded_uri).wait();
            encoded_uri = uri::encode_uri(U("/path 1/path 2#fragment1")); // URI has path and fragment components
            client.request(methods::GET, encoded_uri).wait();
            encoded_uri = uri::encode_uri(
                U("/path 1/path 2?key1=val1 val2#fragment1")); // URI has path, query and fragment components
            client.request(methods::GET, encoded_uri).wait();
        }
        catch (const http_exception& e)
        {
            std::cout << "http_exception caught" << std::endl
                      << "what():" << e.what() << std::endl
                      << "error_code msg:" << e.error_code().message() << std::endl
                      << "error_code value:" << e.error_code().value() << std::endl;
            VERIFY_IS_TRUE(false);
        }

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, https_listener, "Ignore", "Manual")
    {
        // Requires a certificate for execution.
        // Here are instructions for creating a self signed cert. Full instructions can be located here:
        // http://blogs.msdn.com/b/haoxu/archive/2009/04/30/one-time-set-up-for-wwsapi-security-examples.aspx
        // From an elevated admin prompt:
        // 1. MakeCert.exe -ss Root -sr LocalMachine -n "CN=Fake-Test-CA" -cy authority -r -sk "CAKeyContainer"
        // 2. MakeCert.exe -ss My -sr LocalMachine -n "CN=localhost" -sky exchange -is Root -ir LocalMachine -in
        // Fake-Test-CA -sk "ServerKeyContainer"
        // 3. Find corresponding SHA-1 hash with CertUtil.exe -store My localhost
        // 4. Netsh.exe http add sslcert ipport=0.0.0.0:8443 appid={00112233-4455-6677-8899-AABBCCDDEEFF}
        // certhash=<40CharacterThumbprintWithNoSpaces>

        http_listener listener(m_secure_uri);
        listener.open().wait();
        client::http_client client(m_secure_uri);

        listener.support([&](http_request request) { request.reply(status_codes::OK); });

        http_asserts::assert_response_equals(client.request(methods::GET, U("")).get(), status_codes::OK);

        listener.close().wait();
    }
}

} // namespace listener
} // namespace http
} // namespace functional
} // namespace tests
