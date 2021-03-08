/***
 * ==++==
 *
 * Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 * ==--==
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * http_listener_tests.h
 *
 * Common declarations and helper functions for http_listener test cases.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#include "cpprest/http_listener.h"

namespace tests
{
namespace functional
{
namespace http
{
namespace listener
{
class uri_address
{
public:
    uri_address() : m_uri(U("http://localhost:34567/")), m_secure_uri(U("https://localhost:8443/"))
    {
        if (!s_dummy_listener)
            s_dummy_listener =
                std::make_shared<web::http::experimental::listener::http_listener>(U("http://localhost:30000/"));
    }

    // By introducing an additional listener, we can avoid having to close the
    // server after each unit test.

    static std::shared_ptr<web::http::experimental::listener::http_listener> s_dummy_listener;
    web::http::uri m_uri;
    web::http::uri m_secure_uri;
};

} // namespace listener
} // namespace http
} // namespace functional
} // namespace tests
