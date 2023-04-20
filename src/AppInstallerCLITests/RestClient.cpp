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
    std::optional<Version> actual = RestClient::GetLatestCommonVersion(versions, wingetSupportedContracts);
    REQUIRE(actual);
    REQUIRE(actual.value().ToString() == "1.2.0");
}

TEST_CASE("GetLatestCommonVersion_OnlyMajorMinorVersionMatched", "[RestSource]")
{
    std::set<AppInstaller::Utility::Version> wingetSupportedContracts = { Version {"1.0.0"}, Version {"1.2.0"} };
    std::vector<std::string> versions{ "1.0.0", "2.0.0", "1.2.1" };
    std::optional<Version> actual = RestClient::GetLatestCommonVersion(versions, wingetSupportedContracts);
    REQUIRE(actual);
    REQUIRE(actual.value().ToString() == "1.2.0");
}

TEST_CASE("GetLatestCommonVersion_UnsupportedVersion", "[RestSource]")
{
    std::set<AppInstaller::Utility::Version> wingetSupportedContracts = { Version {"3.0.0"}, Version {"4.2.0"} };
    std::vector<std::string> versions{ "1.0.0", "2.0.0" };
    std::optional<Version> actual = RestClient::GetLatestCommonVersion(versions, wingetSupportedContracts);
    REQUIRE(!actual);
}

TEST_CASE("GetSupportedInterface", "[RestSource]")
{
    IRestClient::Information info{ "TestId", { "1.0.0" } };

    Version version{ "1.0.0" };
    REQUIRE(RestClient::GetSupportedInterface(utility::conversions::to_utf8string(TestRestUri), {}, info, version)->GetVersion() == version);

    // Update this test to next version so that we don't forget to add to supported versions before rest e2e tests are available.
    Version invalid{ "1.5.0" };
    REQUIRE_THROWS(RestClient::GetSupportedInterface(utility::conversions::to_utf8string(TestRestUri), {}, info, invalid));
}

TEST_CASE("GetInformation_Success", "[RestSource]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.0.0",
                "1.1.0"],
              "SourceAgreements": {
                "AgreementsIdentifier": "agreementV1",
                "Agreements": [{
                    "AgreementLabel": "EULA",
                    "Agreement": "this is store agreement",
                    "AgreementUrl": "https://store.agreement"
                  }
                ]
              },
              "RequiredQueryParameters": [
                "Market"
              ],
              "RequiredPackageMatchFields": [
                "Market"
              ],
              "UnsupportedQueryParameters": [
                "Moniker"
              ],
              "UnsupportedPackageMatchFields": [
                "Moniker"
              ]
        }})delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, sample) };
    IRestClient::Information information = RestClient::GetInformation(TestRestUri, {}, std::move(helper));
    REQUIRE(information.SourceIdentifier == "Source123");
    REQUIRE(information.ServerSupportedVersions.size() == 2);
    REQUIRE(information.ServerSupportedVersions.at(0) == "1.0.0");
    REQUIRE(information.ServerSupportedVersions.at(1) == "1.1.0");
    REQUIRE(information.SourceAgreementsIdentifier == "agreementV1");
    REQUIRE(information.SourceAgreements.size() == 1);
    REQUIRE(information.SourceAgreements.at(0).Label == "EULA");
    REQUIRE(information.SourceAgreements.at(0).Text == "this is store agreement");
    REQUIRE(information.SourceAgreements.at(0).Url == "https://store.agreement");
    REQUIRE(information.RequiredQueryParameters.size() == 1);
    REQUIRE(information.RequiredQueryParameters.at(0) == "Market");
    REQUIRE(information.RequiredPackageMatchFields.size() == 1);
    REQUIRE(information.RequiredPackageMatchFields.at(0) == "Market");
    REQUIRE(information.UnsupportedQueryParameters.size() == 1);
    REQUIRE(information.UnsupportedQueryParameters.at(0) == "Moniker");
    REQUIRE(information.UnsupportedPackageMatchFields.size() == 1);
    REQUIRE(information.UnsupportedPackageMatchFields.at(0) == "Moniker");
}

TEST_CASE("GetInformation_Fail_AgreementsWithoutIdentifier", "[RestSource]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.0.0",
                "1.1.0"],
              "SourceAgreements": {
                "Agreements": [{
                    "AgreementLabel": "EULA",
                    "Agreement": "this is store agreement",
                    "AgreementUrl": "https://store.agreement"
                  }
                ]
              }
        }})delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, sample) };
    REQUIRE_THROWS_HR(RestClient::GetInformation(TestRestUri, {}, std::move(helper)), APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE);
}

TEST_CASE("RestClientCreate_UnsupportedVersion", "[RestSource]")
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
    REQUIRE_THROWS_HR(RestClient::Create("https://restsource.com/api", {}, {}, std::move(helper)), APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE);
}

TEST_CASE("RestClientCreate_1.0_Success", "[RestSource]")
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
    RestClient client = RestClient::Create(utility::conversions::to_utf8string(TestRestUri), {}, {}, std::move(helper));
    REQUIRE(client.GetSourceIdentifier() == "Source123");
}

TEST_CASE("RestClientCreate_1.1_Success", "[RestSource]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.0.0",
                "1.1.0"],
              "SourceAgreements": {
                "AgreementsIdentifier": "agreementV1",
                "Agreements": [{
                    "AgreementLabel": "EULA",
                    "Agreement": "this is store agreement",
                    "AgreementUrl": "https://store.agreement"
                  }
                ]
              },
              "RequiredQueryParameters": [
                "Market"
              ],
              "RequiredPackageMatchFields": [
                "Market"
              ],
              "UnsupportedQueryParameters": [
                "Moniker"
              ],
              "UnsupportedPackageMatchFields": [
                "Moniker"
              ]
        }})delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, sample) };
    RestClient client = RestClient::Create(utility::conversions::to_utf8string(TestRestUri), {}, {}, std::move(helper));
    REQUIRE(client.GetSourceIdentifier() == "Source123");
    auto information = client.GetSourceInformation();
    REQUIRE(information.SourceAgreementsIdentifier == "agreementV1");
    REQUIRE(information.SourceAgreements.size() == 1);
    REQUIRE(information.SourceAgreements.at(0).Label == "EULA");
    REQUIRE(information.SourceAgreements.at(0).Text == "this is store agreement");
    REQUIRE(information.SourceAgreements.at(0).Url == "https://store.agreement");
    REQUIRE(information.RequiredQueryParameters.size() == 1);
    REQUIRE(information.RequiredQueryParameters.at(0) == "Market");
    REQUIRE(information.RequiredPackageMatchFields.size() == 1);
    REQUIRE(information.RequiredPackageMatchFields.at(0) == "Market");
    REQUIRE(information.UnsupportedQueryParameters.size() == 1);
    REQUIRE(information.UnsupportedQueryParameters.at(0) == "Moniker");
    REQUIRE(information.UnsupportedPackageMatchFields.size() == 1);
    REQUIRE(information.UnsupportedPackageMatchFields.at(0) == "Moniker");
}
