// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <Rest/RestClient.h>
#include <Rest/Schema/IRestClient.h>
#include <AppInstallerVersions.h>
#include <set>

using namespace AppInstaller;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Repository::Rest;
using namespace AppInstaller::Repository::Rest::Schema;

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
    REQUIRE(RestClient::GetSupportedInterface("https://restsource.net", version)->GetVersion() == version);

    Version invalid{ "1.2.0" };
    REQUIRE_THROWS(RestClient::GetSupportedInterface("https://restsource.net", invalid));
}

TEST_CASE("GetSupportedVersion", "[RestSource]")
{
    class customHandler : public web::http::http_pipeline_stage
    {
    public:
        virtual pplx::task<web::http::http_response> propagate(web::http::http_request request)
        {
            web::json::value data;
            data[L"SourceIdentifier"] = web::json::value::string(L"Source123");
            web::json::value versions = web::json::value::array();
            versions[0] = web::json::value::string(L"0.2.0");
            data[L"ServerSupportedVersions"] = versions;
            web::json::value result;
            result[L"Data"] = data;

            web::http::http_response response;
            response.set_body(result);
            response.set_status_code(web::http::status_codes::OK);
            return pplx::task_from_result(response);
        }
    };

    HttpClientHelper helper{ std::make_shared<customHandler>() };
    utility::string_t restApi = L"http://restsource.net";
    std::set<AppInstaller::Utility::Version> wingetSupportedContracts = { Version {"1.3.0"}, Version {"0.2.0"}, Version {"1.2.0"} };
    REQUIRE(RestClient::GetSupportedVersion(restApi, wingetSupportedContracts, std::move(helper)) == Version{ "0.2.0" });
}

