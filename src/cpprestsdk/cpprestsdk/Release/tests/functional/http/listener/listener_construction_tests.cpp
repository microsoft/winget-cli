/***
 * ==++==
 *
 * Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 * ==--==
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * listener_construction_tests.cpp
 *
 * Tests cases for covering creating http_listeners in various ways.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

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
SUITE(listener_construction_tests)
{
    TEST_FIXTURE(uri_address, default_constructor)
    {
        // Test that the default ctor works.
        http_listener listener;

        VERIFY_IS_TRUE(listener.uri().is_empty());
        VERIFY_THROWS(listener.open().wait(), std::invalid_argument);
    }

    TEST_FIXTURE(uri_address, move_operations)
    {
        http_listener listener(m_uri);
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        // move constructor
        http_listener listener2 = std::move(listener);
        listener2.support(methods::PUT, [](http_request request) {
            http_asserts::assert_request_equals(request, U("PUT"), U("/"));
            request.reply(status_codes::OK);
        });
        VERIFY_ARE_EQUAL(0, p_client->request(U("PUT"), U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        // move assignment
        listener = std::move(listener2);
        listener.support(methods::PUT, [](http_request request) {
            http_asserts::assert_request_equals(request, U("PUT"), U("/"));
            request.reply(status_codes::OK);
        });
        VERIFY_ARE_EQUAL(0, p_client->request(U("PUT"), U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, various_uris)
    {
        http_listener listener(web::http::uri_builder(m_uri).append_path(U("path1")).to_uri());
        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        // Path that matches exactly
        listener.support([](http_request request) {
            http_asserts::assert_request_equals(request, U("GET"), U(""));
            request.reply(status_codes::OK);
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/path1/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        // Path that matches but is more specific.
        listener.support([](http_request request) {
            http_asserts::assert_request_equals(request, U("GET"), U("/path2"));
            request.reply(status_codes::OK);
        });
        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/path1/path2")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();

        // Try a request with a path that doesn't match.
        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/path3/path2")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::NotFound);
            })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, uri_routing)
    {
        http_listener listener1(web::http::uri_builder(m_uri).append_path(U("path1")).to_uri());
        http_listener listener2(web::http::uri_builder(m_uri).append_path(U("path2")).to_uri());
        http_listener listener3(web::http::uri_builder(m_uri).append_path(U("path1/path2")).to_uri());

        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        // Path that matches exactly
        listener1.support([](http_request request) { request.reply(status_codes::OK); });
        listener1.open().wait();

        listener2.support([](http_request request) { request.reply(status_codes::Created); });
        listener2.open().wait();

        listener3.support([](http_request request) { request.reply(status_codes::Accepted); });
        listener3.open().wait();

        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/path1/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/path2")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::Created);
            })
            .wait();
        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/path1/path2")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::Accepted);
            })
            .wait();

        // Try a request with a path that doesn't match.
        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/path3/path2")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::NotFound);
            })
            .wait();

        listener1.close().wait();
        listener2.close().wait();
        listener3.close().wait();
    }

    TEST_FIXTURE(uri_address, uri_error_cases)
    {
        // non HTTP scheme
        VERIFY_THROWS(http_listener(U("ftp://localhost:456/")), std::invalid_argument);

        // empty HTTP host
        VERIFY_THROWS(http_listener(U("http://:456/")), std::invalid_argument);

        // try specifying a query
        VERIFY_THROWS(http_listener(U("http://localhost:45678/path?key1=value")), std::invalid_argument);

        // try specifing a fragment
        VERIFY_THROWS(http_listener(U("http://localhost:4563/path?key1=value#frag")), std::invalid_argument);
    }

    TEST_FIXTURE(uri_address, create_listener_get)
    {
        http_listener listener(m_uri);

        listener.support(methods::GET, [](http_request request) {
            http_asserts::assert_request_equals(request, methods::GET, U("/"));
            request.reply(status_codes::OK);
        });

        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        VERIFY_ARE_EQUAL(0, p_client->request(methods::PUT, U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::MethodNotAllowed);
            })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, create_listener_get_put)
    {
        http_listener listener(m_uri);

        listener.support(methods::GET, [](http_request request) {
            http_asserts::assert_request_equals(request, methods::GET, U("/"));
            request.reply(status_codes::OK);
        });

        listener.support(methods::PUT, [](http_request request) {
            http_asserts::assert_request_equals(request, methods::PUT, U("/"));
            request.reply(status_codes::OK);
        });

        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        VERIFY_ARE_EQUAL(0, p_client->request(methods::PUT, U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        VERIFY_ARE_EQUAL(0, p_client->request(methods::POST, U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::MethodNotAllowed);
            })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, create_listener_get_put_post)
    {
        http_listener listener(m_uri);

        listener.support(methods::GET, [](http_request request) {
            http_asserts::assert_request_equals(request, methods::GET, U("/"));
            request.reply(status_codes::OK);
        });

        listener.support(methods::PUT, [](http_request request) {
            http_asserts::assert_request_equals(request, methods::PUT, U("/"));
            request.reply(status_codes::OK);
        });

        listener.support(methods::POST, [](http_request request) {
            http_asserts::assert_request_equals(request, methods::POST, U("/"));
            request.reply(status_codes::OK);
        });

        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        VERIFY_ARE_EQUAL(0, p_client->request(methods::PUT, U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        VERIFY_ARE_EQUAL(0, p_client->request(methods::POST, U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        VERIFY_ARE_EQUAL(0, p_client->request(methods::DEL, U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::MethodNotAllowed);
            })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, create_listener_get_put_post_delete)
    {
        http_listener listener(m_uri);

        listener.support(methods::GET, [](http_request request) {
            http_asserts::assert_request_equals(request, methods::GET, U("/"));
            request.reply(status_codes::OK);
        });

        listener.support(methods::PUT, [](http_request request) {
            http_asserts::assert_request_equals(request, methods::PUT, U("/"));
            request.reply(status_codes::OK);
        });

        listener.support(methods::POST, [](http_request request) {
            http_asserts::assert_request_equals(request, methods::POST, U("/"));
            request.reply(status_codes::OK);
        });

        listener.support(methods::DEL, [](http_request request) {
            http_asserts::assert_request_equals(request, methods::DEL, U("/"));
            request.reply(status_codes::OK);
        });

        listener.open().wait();
        test_http_client::scoped_client client(m_uri);
        test_http_client* p_client = client.client();

        VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        VERIFY_ARE_EQUAL(0, p_client->request(methods::PUT, U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        VERIFY_ARE_EQUAL(0, p_client->request(methods::POST, U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        VERIFY_ARE_EQUAL(0, p_client->request(methods::DEL, U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::OK);
            })
            .wait();
        VERIFY_ARE_EQUAL(0, p_client->request(methods::HEAD, U("/")));
        p_client->next_response()
            .then([](test_response* p_response) {
                http_asserts::assert_test_response_equals(p_response, status_codes::MethodNotAllowed);
            })
            .wait();

        listener.close().wait();
    }

    TEST_FIXTURE(uri_address, get_listener_config)
    {
        // Verify default configuration.
        {
            http_listener listener(m_uri);
            VERIFY_ARE_EQUAL(utility::seconds(120), listener.configuration().timeout());
            listener.open().wait();
            listener.close().wait();
        }

        // Verify specified config values.
        {
            http_listener_config config;
            utility::seconds t(1);
            config.set_timeout(t);
            http_listener listener(m_uri, config);
            listener.open().wait();
            listener.close().wait();
            VERIFY_ARE_EQUAL(t, listener.configuration().timeout());
        }
    }

    TEST_FIXTURE(uri_address, listener_config_creation)
    {
        // copy constructor
        {
            http_listener_config config;
            config.set_timeout(utility::seconds(2));
            http_listener_config copy(config);
            VERIFY_ARE_EQUAL(utility::seconds(2), copy.timeout());
        }

        // move constructor
        {
            http_listener_config config;
            config.set_timeout(utility::seconds(2));
            http_listener_config ctorMove(std::move(config));
            VERIFY_ARE_EQUAL(utility::seconds(2), ctorMove.timeout());
        }

        // assignment
        {
            http_listener_config config;
            config.set_timeout(utility::seconds(2));
            http_listener_config assign;
            assign = config;
            VERIFY_ARE_EQUAL(utility::seconds(2), assign.timeout());
        }

        // move assignment
        {
            http_listener_config config;
            config.set_timeout(utility::seconds(2));
            http_listener_config assignMove;
            assignMove = std::move(config);
            VERIFY_ARE_EQUAL(utility::seconds(2), assignMove.timeout());
        }
    }

#if !defined(_WIN32) && !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)

    TEST_FIXTURE(uri_address, create_https_listener_get, "Ignore", "github 209")
    {
        const char* self_signed_cert = R"(
-----BEGIN CERTIFICATE-----
MIIDlzCCAn+gAwIBAgIJAP9ZV+1X94UjMA0GCSqGSIb3DQEBBQUAMGIxCzAJBgNV
BAYTAkNOMQswCQYDVQQIDAJTSDELMAkGA1UEBwwCU0gxEjAQBgNVBAoMCU1JQ1JP
U09GVDERMA8GA1UECwwISFBDLVBBQ0sxEjAQBgNVBAMMCWxvY2FsaG9zdDAeFw0x
NTA4MTkwOTA0MjhaFw00MzAxMDMwOTA0MjhaMGIxCzAJBgNVBAYTAkNOMQswCQYD
VQQIDAJTSDELMAkGA1UEBwwCU0gxEjAQBgNVBAoMCU1JQ1JPU09GVDERMA8GA1UE
CwwISFBDLVBBQ0sxEjAQBgNVBAMMCWxvY2FsaG9zdDCCASIwDQYJKoZIhvcNAQEB
BQADggEPADCCAQoCggEBALLv7AAPa+4wYpa+3tqc9HIHhh8kv/MpV2Dm+oKG27iH
zOugMNAPqLzMAaWCzDRyw27I+jPS3pzAAu6rQ0v2H6XNrie1YEEV27j1WOUS9iFy
vcf6Y+ywUKXvFlN/VM/ZFz9Z8U3jc7Y6unIyoUs8UdX/RRITspb2m7SUxlmLJ+4c
qiLrHwstNB2NHIZN72oc8DaS5eBqBdT9h6NO62RSBTrAlR7Vk9eU/5trYkd5+PoC
pispvU+7Fe24uVerGgU6Yoyd7DMj+3BpbG3g/VkOlGhgH0DNtbKu3v/XOmnzdZn6
dzoOoGFNpG1NeH2Xv0vnvEZP6WG4h/TFSafBJMONNnMCAwEAAaNQME4wHQYDVR0O
BBYEFO1mAjAmLk1J0iT93xfczAE5mxgzMB8GA1UdIwQYMBaAFO1mAjAmLk1J0iT9
3xfczAE5mxgzMAwGA1UdEwQFMAMBAf8wDQYJKoZIhvcNAQEFBQADggEBAFB8AACf
5O+sPe3PZ8IPgwZb+BCXdoXc2rngR/gOaYO019TZyNLuHRzW9FtplzW25IbQ9Jnc
b+jmY2Ill7Zf3TX4OhHEwscJ1G2LBaqZfQlwSbYJmCzvRNSzSbF3RigNQD5Qhdph
vVBdvVGTZnVeatjTOFKUyfhcXf4DMb6eMfaU6il/VJCSMW0j3hYNQjPm3V/PLxnG
fd9T4hpCUd8MK2XG4RqJAzh6x/6v0fc6mRHBS5+qTWYSDGFwITrU1pP2L9qFegpm
aNAom7bdENU8uivd+vrLnG2fKvFSssjVfaXpFLKAICfTJY9A3/CWnZ1AcbE5El7A
adctopihoUrlAb0=
-----END CERTIFICATE-----
        )";
        const char* private_key = R"(
-----BEGIN PRIVATE KEY-----
MIIEvwIBADANBgkqhkiG9w0BAQEFAASCBKkwggSlAgEAAoIBAQCy7+wAD2vuMGKW
vt7anPRyB4YfJL/zKVdg5vqChtu4h8zroDDQD6i8zAGlgsw0csNuyPoz0t6cwALu
q0NL9h+lza4ntWBBFdu49VjlEvYhcr3H+mPssFCl7xZTf1TP2Rc/WfFN43O2Orpy
MqFLPFHV/0USE7KW9pu0lMZZiyfuHKoi6x8LLTQdjRyGTe9qHPA2kuXgagXU/Yej
TutkUgU6wJUe1ZPXlP+ba2JHefj6AqYrKb1PuxXtuLlXqxoFOmKMnewzI/twaWxt
4P1ZDpRoYB9AzbWyrt7/1zpp83WZ+nc6DqBhTaRtTXh9l79L57xGT+lhuIf0xUmn
wSTDjTZzAgMBAAECggEAenzd8lScL1qTwlk6ODAE7SHVX/BKLWv5Um4KwdsLAVCE
qC7p+yMdANAtuFzG6Ig+29Fb5KnOlUKjPzmhQZhjpZ4cPzZbg3IxDHV2uqi2L8NZ
wlDWoik3q770a4fYSMd0sHsjQYwXo4CkLJQX8WaDJpgtcehl8g0yHPVSqe0mEkoL
dxdqaZnxprchscxefWaGaysIxEO+V+ZOBaPNf4i8PmBKoMNczWZbLcdKhRL7aLeW
ngPQp1xSWYoN8fPoonpL2qTSop3Nsc2INpwGcYPAj3vxdasC3+DZ8JEJI2AmxpVB
13BLkd3nDzOwimZIlu9Fv+NMJ1vb9XdC249ZOqo68QKBgQDigkws1W429nqDtEtQ
Dr5ebHTdP4gZlNt6vWx5obGLCMBAzoyubfNCCBTCYsCPj8hXxNfiPArPFFkIgEx9
+w0n7BlaYL6SD2xD4q+YzA1/j4Loakxc7N9z8Cyu+/YHifvLhzwqgFnkLfFnVq9N
TF8TatHUYcrbcpawJLz0wr/cnQKBgQDKPAYNTzqPLOOBaE4DfnJNn2zctGU8G5Xp
0L/ED8O1t9AjjV2xVO8PDPNDZAxMzgnIbWeU9iWRSLbr7NloXElKh/QlITjAbSXe
HsUruq1SmDgiaUhEtDaaJ1SqSZZWY2BZqNXMdILOCgvZGnOyyBR2U49zuNaRHyhm
kmZMdIIKTwKBgQDezAk/hEQfvfuuNpZpzcbEu+uLgKVPfFMSfOYJEdnAB0CLvl80
Z6QBzE8XEOmVjHkkk9NBjYuYOsyEhyY2OM2s+hfKBSUOKCt27q+IHRYd5bx+/afV
M41rzc8141ISAlBw1rmAmLVSszojSmmuH7PZNpXkULineCPuaISQQEtWJQKBgQDD
laVsvdEuowUsJEo+ys2VELhiAv1dUnh79u1fmrd2SV085P1WAYRqE+Y4qMvUg/em
JVjmEeBnT+HI7fmdGpOvRyjxt92BDI5w8WVTU2lI1fqEHTpNZ9Te5WbWgfCpf9ax
H74VzCCtT74Bq7l1kFdp0IqOKpcpJu8VtETHcG5LtQKBgQC4Tx7El1Xb4hsI4dvE
h43j3KBb3evlz6vaqgz0BArahYAz2UkkOYDSOPs4K6aOxxXjO0BjqQqCx/tCPcU5
AvLsTlswO+wDLXM1DoKxzFBZL5o8927niqW+vZpzyGc1uPmC1MG7+MDKdZsR+e+9
XzJTD4slrGSJrcpLt/g/Jqqdjg==
-----END PRIVATE KEY-----
        )";

        auto body = utility::string_t {U("body content")};
        http_headers all_headers;
        all_headers.add(U("Accept"), U("text/plain"));
        all_headers.add(U("Accept-Charset"), U("utf-8"));
        all_headers.add(U("Accept-Encoding"), U("gzip, deflate"));
        all_headers.add(U("Accept-Language"), U("en-US"));
        all_headers.add(U("Accept-Datetime"), U("Thu, 31 May 2007 20:35:00 GMT"));
        all_headers.add(U("Authorization"), U("Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ=="));
        all_headers.add(U("Cache-Control"), U("no-cache"));
        all_headers.add(U("Cookie"), U("$Version=1; Skin=new;"));
        all_headers.add(U("Content-Length"), body.size());
        all_headers.add(U("Content-MD5"), U("Q2hlY2sgSW50ZWdyaXR5IQ=="));
        all_headers.add(U("Content-Type"), U("application/x-www-form-urlencoded"));
        all_headers.add(U("Date"), U("Tue, 15 Nov 1994 08:12:31 GMT"));
        all_headers.add(U("Expect"), U("100-continue"));
        all_headers.add(U("Forwarded"),
                        U("for=192.0.2.60;proto=http;by=203.0.113.43Forwarded: for=192.0.2.43, for=198.51.100.17"));
        all_headers.add(U("From"), U("user@example.com"));
        all_headers.add(U("Host"), U("en.wikipedia.org"));
        all_headers.add(U("If-Match"), U("\"737060cd8c284d8af7ad3082f209582d\""));
        all_headers.add(U("If-Modified-Since"), U("Sat, 29 Oct 1994 19:43:31 GMT"));
        all_headers.add(U("If-None-Match"), U("\"737060cd8c284d8af7ad3082f209582d\""));
        all_headers.add(U("If-Range"), U("\"737060cd8c284d8af7ad3082f209582d\""));
        all_headers.add(U("If-Unmodified-Since"), U("Sat, 29 Oct 1994 19:43:31 GMT"));
        all_headers.add(U("Max-Forwards"), U("10"));
        all_headers.add(U("Origin"), U("http://www.example-social-network.com"));
        all_headers.add(U("Pragma"), U("no-cache"));
        all_headers.add(U("Proxy-Authorization"), U("Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ=="));
        all_headers.add(U("Range"), U("bytes=500-999"));
        all_headers.add(U("Referer"), U("http://en.wikipedia.org/wiki/Main_Page"));
        all_headers.add(U("TE"), U("trailers, deflate"));
        all_headers.add(U("User-Agent"), U("Mozilla/5.0 (X11; Linux x86_64; rv:12.0) Gecko/20100101 Firefox/21.0"));
        all_headers.add(U("Upgrade"), U("HTTP/2.0, SHTTP/1.3, IRC/6.9, RTA/x11"));
        all_headers.add(U("Via"), U("1.0 fred, 1.1 example.com (Apache/1.1)"));
        all_headers.add(U("Warning"), U("199 Miscellaneous warning"));

        boost::asio::const_buffer cert(self_signed_cert, std::strlen(self_signed_cert));
        boost::asio::const_buffer key(private_key, std::strlen(private_key));

        http_listener_config server_config;
        server_config.set_ssl_context_callback([&](boost::asio::ssl::context& ctx) {
            ctx.set_options(boost::asio::ssl::context::default_workarounds);
            ctx.use_certificate_chain(cert);
            ctx.use_private_key(key, boost::asio::ssl::context::pem);
        });

        http_listener listener(m_secure_uri, server_config);

        listener.support(methods::GET, [&](http_request request) {
            http_asserts::assert_request_equals(request, methods::GET, U("/"));

            for (auto&& h : all_headers)
            {
                VERIFY_IS_TRUE(request.headers().has(h.first));
                VERIFY_ARE_EQUAL(h.second, request.headers().find(h.first)->second);
            }

            VERIFY_ARE_EQUAL(body, request.extract_string(true).get());

            request.reply(status_codes::OK);
        });

        listener.open().wait();

        client::http_client_config client_config;
#if !defined(_WIN32) && !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
        client_config.set_ssl_context_callback(
            [&](boost::asio::ssl::context& ctx) { ctx.add_certificate_authority(cert); });
#else
        // in this build configuration, with WinHTTP-based http_client, this test will fail unless the self-signed
        // cert is added to the Windows certificate store (or certificate validation is disabled in client_config)
#endif
        client::http_client client(m_secure_uri, client_config);
        http_request msg(methods::GET);
        msg.set_request_uri(U("/"));

        msg.headers() = all_headers;
        msg.set_body(body);

        http_asserts::assert_response_equals(client.request(msg).get(), status_codes::OK);

        listener.close().wait();
    }
#endif
}

} // namespace listener
} // namespace http
} // namespace functional
} // namespace tests
