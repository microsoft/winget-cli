// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestRestRequestHandler.h"
#include <Rest/RestClient.h>
#include <Rest/Schema/IRestClient.h>
#include <AppInstallerVersions.h>
#include <set>
#include <AppInstallerErrors.h>

using namespace AppInstaller;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Repository::Rest;
using namespace AppInstaller::Repository::Rest::Schema;

const utility::string_t TestRestUri = L"http://restsource.net";

TEST_CASE("GetLatestCommonVersion", "[RestSource]")
{
    std::set<AppInstaller::Utility::Version> wingetSupportedContracts = { Version {"1.0.0"}, Version {"1.2.0"} };
    std::vector<std::string> versions{ "1.0.0", "2.0.0", "1.2.0" };
    IRestClient::Information info{ "SourceIdentifier", std::move(versions) };
    std::optional<Version> actual = RestClient::GetLatestCommonVersion(info, wingetSupportedContracts);
    REQUIRE(actual);
    REQUIRE(actual.value().ToString() == "1.2.0");
}

TEST_CASE("GetLatestCommonVersion_UnsupportedVersion", "[RestSource]")
{
    std::set<AppInstaller::Utility::Version> wingetSupportedContracts = { Version {"3.0.0"}, Version {"4.2.0"} };
    std::vector<std::string> versions{ "1.0.0", "2.0.0" };
    IRestClient::Information info{ "SourceIdentifier", std::move(versions) };
    std::optional<Version> actual = RestClient::GetLatestCommonVersion(info, wingetSupportedContracts);
    REQUIRE(!actual);
}

TEST_CASE("GetSupportedInterface", "[RestSource]")
{
    Version version{ "1.0.0" };
    REQUIRE(RestClient::GetSupportedInterface(utility::conversions::to_utf8string(TestRestUri), version)->GetVersion() == version);

    Version invalid{ "1.2.0" };
    REQUIRE_THROWS(RestClient::GetSupportedInterface(utility::conversions::to_utf8string(TestRestUri), invalid));
}

TEST_CASE("GetSupportedVersion_Success", "[RestSource]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "0.2.0",
                "1.0.0"]
        }})delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, sample) };
    std::set<AppInstaller::Utility::Version> wingetSupportedContracts = { Version {"1.3.0"}, Version {"0.2.0"}, Version {"1.10.0"} };
    REQUIRE(RestClient::GetSupportedVersion(TestRestUri, wingetSupportedContracts, std::move(helper)) == Version{ "0.2.0" });
}

TEST_CASE("GetSupportedVersion_UnexpectedVersion", "[RestSource]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.2.0",
                "2.0.0"]
        }})delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, sample) };
    std::set<AppInstaller::Utility::Version> wingetSupportedContracts = { Version {"1.0.0"}, Version {"0.2.0"} };
    REQUIRE_THROWS_HR(RestClient::GetSupportedVersion(TestRestUri, wingetSupportedContracts, std::move(helper)),
        APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE);
}

TEST_CASE("RestClientCreate", "[RestSource]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.0.0",
                "2.0.0"]
        }})delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, sample) };
    REQUIRE_NOTHROW(RestClient::Create(utility::conversions::to_utf8string(TestRestUri), std::move(helper)));
}
