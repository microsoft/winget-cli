// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "../../util/SFSExceptionMatcher.h"
#include "../../util/TestHelper.h"
#include "ReportingHandler.h"
#include "SFSUrlBuilder.h"

#include <catch2/catch_test_macros.hpp>

#define TEST(...) TEST_CASE("[SFSUrlBuilderTests] " __VA_ARGS__)

using namespace SFS::details;
using namespace SFS::test;

const std::string c_accountId = "accountId";
const std::string c_instanceId = "instanceId";
const std::string c_nameSpace = "nameSpace";

TEST("SFSUrlBuilder")
{
    ReportingHandler handler;
    handler.SetLoggingCallback(LogCallbackToTest);

    SECTION("CreateFromAccountId()")
    {
        SECTION("ASCII strings")
        {
            SFSUrlBuilder builder(c_accountId, c_instanceId, c_nameSpace, handler);
            REQUIRE(builder.GetUrl() == "https://accountId.api.cdp.microsoft.com/");

            REQUIRE(
                builder.GetLatestVersionUrl("product") ==
                "https://accountId.api.cdp.microsoft.com/api/v2/contents/instanceId/namespaces/nameSpace/names/product/versions/latest?action=select");

            REQUIRE(
                builder.GetLatestVersionBatchUrl() ==
                "https://accountId.api.cdp.microsoft.com/api/v2/contents/instanceId/namespaces/nameSpace/names?action=BatchUpdates");

            REQUIRE(
                builder.GetSpecificVersionUrl("product", "version") ==
                "https://accountId.api.cdp.microsoft.com/api/v2/contents/instanceId/namespaces/nameSpace/names/product/versions/version");

            REQUIRE(
                builder.GetDownloadInfoUrl("product", "version") ==
                "https://accountId.api.cdp.microsoft.com/api/v2/contents/instanceId/namespaces/nameSpace/names/product/versions/version/files?action=GenerateDownloadInfo");
        }

        SECTION("Non-ASCII strings")
        {
            REQUIRE_THROWS_CODE_MSG(SFSUrlBuilder("a&b", c_instanceId, c_nameSpace, handler),
                                    ConnectionUrlSetupFailed,
                                    "Curl URL error: Bad hostname");

            REQUIRE_THROWS_CODE_MSG(SFSUrlBuilder("a\nb", c_instanceId, c_nameSpace, handler),
                                    ConnectionUrlSetupFailed,
                                    "Curl URL error: Bad hostname");

            REQUIRE_THROWS_CODE_MSG(SFSUrlBuilder("a\tb", c_instanceId, c_nameSpace, handler),
                                    ConnectionUrlSetupFailed,
                                    "Curl URL error: Bad hostname");

            SFSUrlBuilder builder(c_accountId, "instanceId@", "namespace+", handler);
            REQUIRE(builder.GetUrl() == "https://accountId.api.cdp.microsoft.com/");

            REQUIRE(
                builder.GetLatestVersionUrl("pr$duct") ==
                "https://accountId.api.cdp.microsoft.com/api/v2/contents/instanceId%40/namespaces/namespace%2b/names/pr%24duct/versions/latest?action=select");

            REQUIRE(
                builder.GetLatestVersionBatchUrl() ==
                "https://accountId.api.cdp.microsoft.com/api/v2/contents/instanceId%40/namespaces/namespace%2b/names?action=BatchUpdates");

            REQUIRE(
                builder.GetSpecificVersionUrl("pr$duct", "versi/n") ==
                "https://accountId.api.cdp.microsoft.com/api/v2/contents/instanceId%40/namespaces/namespace%2b/names/pr%24duct/versions/versi%2fn");

            REQUIRE(
                builder.GetDownloadInfoUrl("pr$duct", "versi/n") ==
                "https://accountId.api.cdp.microsoft.com/api/v2/contents/instanceId%40/namespaces/namespace%2b/names/pr%24duct/versions/versi%2fn/files?action=GenerateDownloadInfo");
        }
    }

    SECTION("CreateFromCustomUrl()")
    {
        SFSUrlBuilder builder(SFSCustomUrl("http://www.example.com"), c_instanceId, c_nameSpace, handler);
        REQUIRE(builder.GetUrl() == "http://www.example.com/");

        SFSUrlBuilder builder2(SFSCustomUrl("http://www.example2.com"), c_instanceId, c_nameSpace, handler);
        REQUIRE(builder2.GetUrl() == "http://www.example2.com/");

        REQUIRE_THROWS_CODE_MSG(SFSUrlBuilder(SFSCustomUrl("http://www.+.com"), c_instanceId, c_nameSpace, handler),
                                ConnectionUrlSetupFailed,
                                "Curl URL error: Bad hostname");

        REQUIRE_THROWS_CODE_MSG(SFSUrlBuilder(SFSCustomUrl("example"), c_instanceId, c_nameSpace, handler),
                                ConnectionUrlSetupFailed,
                                "Curl URL error: Bad scheme");
    }
}
