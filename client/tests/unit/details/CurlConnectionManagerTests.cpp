// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ReportingHandler.h"
#include "connection/Connection.h"
#include "connection/CurlConnection.h"
#include "connection/CurlConnectionManager.h"

#include <catch2/catch_test_macros.hpp>
#include <curl/curl.h>

using namespace SFS;
using namespace SFS::details;

#define TEST(...) TEST_CASE("[CurlConnectionManagerTests] " __VA_ARGS__)

TEST("Testing expected values in curl_version_info_data")
{
    curl_version_info_data* ver = curl_version_info(CURLVERSION_NOW);
    REQUIRE(ver != nullptr);
    CHECK(ver->features & CURL_VERSION_SSL);

    // Checking thread safety is on
    CHECK(ver->features & CURL_VERSION_THREADSAFE);

    // For thread safety we need the DNS resolutions to be asynchronous (which happens because of c-ares)
    CHECK(ver->features & CURL_VERSION_ASYNCHDNS);
}

TEST("Testing CurlConnectionManager()")
{
    ReportingHandler handler;
    CurlConnectionManager curlConnectionManager(handler);

    // Check that the CurlConnectionManager generates a CurlConnection object
    std::unique_ptr<Connection> Connection = curlConnectionManager.MakeConnection({});
    REQUIRE(Connection != nullptr);
    REQUIRE(dynamic_cast<CurlConnection*>(Connection.get()) != nullptr);

    // Having many CurlConnectionManager objects should not cause any issues, curl is smart enough to
    // handle multiple initialization and cleanup calls
    CurlConnectionManager curlConnectionManager2(handler);
    auto Connection2 = curlConnectionManager2.MakeConnection({});

    CurlConnectionManager curlConnectionManager3(handler);
    auto Connection3 = curlConnectionManager3.MakeConnection({});
    auto Connection4 = curlConnectionManager3.MakeConnection({});
}
