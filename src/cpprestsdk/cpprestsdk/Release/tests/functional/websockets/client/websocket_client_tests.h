/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * websocket_client_tests.h
 *
 * Common declarations and helper functions for http_client test cases.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#include "cpprest/uri.h"
#include "unittestpp.h"

namespace tests
{
namespace functional
{
namespace websocket
{
namespace client
{
class uri_address
{
public:
    uri_address() : m_uri(U("ws://localhost:9980/ws")) {}
    web::uri m_uri;
};

} // namespace client
} // namespace websocket
} // namespace functional
} // namespace tests
