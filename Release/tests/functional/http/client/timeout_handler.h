/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Simple utility for handling timeouts with http client test cases.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#include "cpprest/http_client.h"

namespace tests
{
namespace functional
{
namespace http
{
namespace client
{
// helper function to check if failure is due to timeout.
inline bool is_timeout(const std::string& msg)
{
    if (msg.find("The operation timed out") != std::string::npos /* WinHTTP */ ||
        msg.find("The operation was timed out") != std::string::npos /* IXmlHttpRequest2 */)
    {
        return true;
    }
    return false;
}

template<typename Func>
void handle_timeout(const Func& f)
{
    try
    {
        f();
    }
    catch (const web::http::http_exception& e)
    {
        if (is_timeout(e.what()))
        {
            // Since this test depends on an outside server sometimes it sporadically can fail due to timeouts
            // especially on our build machines.
            return;
        }
        throw;
    }
}

} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
