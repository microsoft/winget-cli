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
