// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestRestRequestHandler.h"
#include <Rest/RestClient.h>
#include <Rest/Schema/IRestClient.h>
#include <AppInstallerVersions.h>
#include <AppInstallerErrors.h>
#include <AppInstallerRuntime.h>

using namespace AppInstaller;
using namespace AppInstaller::Http;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Repository::Rest;
using namespace AppInstaller::Repository::Rest::Schema;

const std::string TestRestUri = "http://restsource.net";

RestClient CreateRestClient(
    const std::string& restApi,
    const std::optional<std::string>& customHeader,
    std::string_view caller,
    const Http::HttpClientHelper& helper,
    const Authentication::AuthenticationArguments& authArgs = {})
{
    return RestClient::Create(restApi, customHeader, caller, helper, RestClient::GetInformation(restApi, customHeader, caller, helper), authArgs);
}

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
    REQUIRE(RestClient::GetSupportedInterface(TestRestUri, {}, info, {}, version, {})->GetVersion() == version);

    // Update this test to next version so that we don't forget to add to supported versions before rest e2e tests are available.
    Version invalid{ "1.11.0" };
    REQUIRE_THROWS_HR(RestClient::GetSupportedInterface(TestRestUri, {}, info, {}, invalid, {}), APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_VERSION);

    Authentication::AuthenticationArguments authArgs;
    authArgs.Mode = Authentication::AuthenticationMode::Silent;
    Version version_1_7{ "1.7.0" };

    // GetSupportedInterface throws on unknown authentication type.
    IRestClient::Information infoWithUnknownAuthenticationType{ "TestId", { "1.7.0" } };
    infoWithUnknownAuthenticationType.Authentication.Type = Authentication::AuthenticationType::Unknown;
    REQUIRE_THROWS_HR(RestClient::GetSupportedInterface(TestRestUri, {}, infoWithUnknownAuthenticationType, authArgs, version_1_7, {}), APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED);

    // GetSupportedInterface throws on invalid authentication info.
    IRestClient::Information infoWithInvalidAuthenticationInfo{ "TestId", { "1.7.0" } };
    infoWithInvalidAuthenticationInfo.Authentication.Type = Authentication::AuthenticationType::MicrosoftEntraId;
    REQUIRE_THROWS_HR(RestClient::GetSupportedInterface(TestRestUri, {}, infoWithInvalidAuthenticationInfo, authArgs, version_1_7, {}), APPINSTALLER_CLI_ERROR_INVALID_AUTHENTICATION_INFO);
}

TEST_CASE("GetInformation_Success", "[RestSource]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.0.0",
                "1.1.0"
               ],
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
    IRestClient::Information information = RestClient::GetInformation(TestRestUri, {}, {}, helper);
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
    REQUIRE(information.Authentication.Type == Authentication::AuthenticationType::None);
    REQUIRE_FALSE(information.Authentication.MicrosoftEntraIdInfo.has_value());
}

TEST_CASE("GetInformation_WithAuthenticationInfo_Success", "[RestSource]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.7.0"
               ],
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
              ],
              "Authentication": {
                "AuthenticationType": "microsoftEntraId",
                "MicrosoftEntraIdAuthenticationInfo" : {
                  "Resource": "GUID",
                  "Scope" : "test"
                }
              }
        }})delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, sample) };
    IRestClient::Information information = RestClient::GetInformation(TestRestUri, {}, {}, helper);
    REQUIRE(information.SourceIdentifier == "Source123");
    REQUIRE(information.ServerSupportedVersions.size() == 1);
    REQUIRE(information.ServerSupportedVersions.at(0) == "1.7.0");
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
    REQUIRE(information.Authentication.Type == Authentication::AuthenticationType::MicrosoftEntraId);
    REQUIRE(information.Authentication.MicrosoftEntraIdInfo.has_value());
    REQUIRE(information.Authentication.MicrosoftEntraIdInfo->Resource == "GUID");
    REQUIRE(information.Authentication.MicrosoftEntraIdInfo->Scope == "test");
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
    REQUIRE_THROWS_HR(RestClient::GetInformation(TestRestUri, {}, {}, helper), APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE);
}

TEST_CASE("GetInformation_Fail_InvalidMicrosoftEntraIdInfo", "[RestSource]")
{
    utility::string_t sample1 = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.7.0"
               ],
              "Authentication": {
                "AuthenticationType": "microsoftEntraId"
              }
        }})delimiter");

    HttpClientHelper helper1{ GetTestRestRequestHandler(web::http::status_codes::OK, sample1) };
    REQUIRE_THROWS_HR(RestClient::GetInformation(TestRestUri, {}, {}, helper1), APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE);

    utility::string_t sample2 = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.7.0"
               ],
              "Authentication": {
                "AuthenticationType": "microsoftEntraId",
                "MicrosoftEntraIdAuthenticationInfo" : {
                  "Resource": "",
                  "Scope" : "test"
                }
              }
        }})delimiter");

    HttpClientHelper helper2{ GetTestRestRequestHandler(web::http::status_codes::OK, sample2) };
    REQUIRE_THROWS_HR(RestClient::GetInformation(TestRestUri, {}, {}, helper2), APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE);

    utility::string_t sample3 = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.7.0"
               ],
              "Authentication": {
                "AuthenticationType": "microsoftEntraId",
                "MicrosoftEntraIdAuthenticationInfo" : {
                  "Scope" : "test"
                }
              }
        }})delimiter");

    HttpClientHelper helper3{ GetTestRestRequestHandler(web::http::status_codes::OK, sample3) };
    REQUIRE_THROWS_HR(RestClient::GetInformation(TestRestUri, {}, {}, helper3), APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE);

    utility::string_t sample4 = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.7.0"
               ],
              "Authentication": {
                "AuthenticationType": "microsoftEntraIdForAzureBlobStorage"
              }
        }})delimiter");

    HttpClientHelper helper4{ GetTestRestRequestHandler(web::http::status_codes::OK, sample4) };
    Authentication::AuthenticationArguments authArgs;
    authArgs.Mode = Authentication::AuthenticationMode::Silent;
    Version version_1_7{ "1.7.0" };
    REQUIRE_THROWS_HR(RestClient::GetSupportedInterface(TestRestUri, {}, RestClient::GetInformation(TestRestUri, {}, {}, helper4), authArgs, version_1_7, {}), APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED);
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
    REQUIRE_THROWS_HR(CreateRestClient("https://restsource.com/api", {}, {}, helper), APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE);
}

TEST_CASE("RestClientCreate_UnsupportedAuthenticationMethod", "[RestSource]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.7.0"
               ],
              "Authentication": {
                "AuthenticationType": "unknown"
              }
        }})delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, sample) };
    Authentication::AuthenticationArguments authArgs;
    authArgs.Mode = Authentication::AuthenticationMode::Silent;
    REQUIRE_THROWS_HR(CreateRestClient("https://restsource.com/api", {}, {}, helper, authArgs), APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED);
}

TEST_CASE("RestClientCreate_InvalidAuthenticationArguments", "[RestSource]")
{
    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.7.0"
               ],
              "Authentication": {
                "AuthenticationType": "microsoftEntraId",
                "MicrosoftEntraIdAuthenticationInfo" : {
                  "Resource" : "test"
                }
              }
        }})delimiter");

    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, sample) };
    Authentication::AuthenticationArguments authArgs;
    authArgs.Mode = Authentication::AuthenticationMode::Unknown;
    REQUIRE_THROWS_HR(CreateRestClient("https://restsource.com/api", {}, {}, helper, authArgs), E_UNEXPECTED);
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
    RestClient client = CreateRestClient(TestRestUri, {}, {}, helper);
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
    RestClient client = CreateRestClient(TestRestUri, {}, {}, helper);
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

TEST_CASE("RestClientCreate_1.7_Success", "[RestSource]")
{
    if (Runtime::IsRunningAsSystem())
    {
        WARN("Test does not support running as system. Skipped.");
        return;
    }

    utility::string_t sample = _XPLATSTR(
        R"delimiter({
            "Data" : {
              "SourceIdentifier": "Source123",
              "ServerSupportedVersions": [
                "1.7.0"
               ],
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
              ],
              "Authentication": {
                "AuthenticationType": "microsoftEntraId",
                "MicrosoftEntraIdAuthenticationInfo" : {
                  "Resource": "GUID",
                  "Scope" : "test"
                }
              }
        }})delimiter");

    Authentication::AuthenticationArguments authArgs;
    authArgs.Mode = Authentication::AuthenticationMode::Silent;
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, sample) };
    RestClient client = CreateRestClient(TestRestUri, {}, {}, helper, authArgs);
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
    REQUIRE(information.Authentication.Type == Authentication::AuthenticationType::MicrosoftEntraId);
    REQUIRE(information.Authentication.MicrosoftEntraIdInfo.has_value());
    REQUIRE(information.Authentication.MicrosoftEntraIdInfo->Resource == "GUID");
    REQUIRE(information.Authentication.MicrosoftEntraIdInfo->Scope == "test");
}
