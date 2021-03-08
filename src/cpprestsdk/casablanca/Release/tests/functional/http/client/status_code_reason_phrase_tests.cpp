/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * status_code_reason_phrase_tests.cpp
 *
 * Tests cases for covering HTTP status codes and reason phrases.
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
SUITE(status_code_reason_phrase_tests)
{
    TEST_FIXTURE(uri_address, status_code)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        // custom status code.
        test_server_utilities::verify_request(&client, methods::GET, U("/"), scoped.server(), 666);
    }

    TEST_FIXTURE(uri_address, reason_phrase)
    {
        test_http_server::scoped_server scoped(m_uri);
        http_client client(m_uri);

        test_server_utilities::verify_request(
            &client, methods::GET, U("/"), scoped.server(), status_codes::OK, U("Reasons!!"));
    }

} // SUITE(status_code_reason_phrase_tests)

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
