// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "Rest/Schema/RestHelper.h"
#include "cpprest/json.h"

using namespace AppInstaller::Repository::Rest::Schema;

TEST_CASE("ValidateAndGetRestAPIBaseUri", "[RestSource]")
{
    REQUIRE(RestHelper::GetRestAPIBaseUri("https://restsource.azurewebsites.net/api/   ") == L"https://restsource.azurewebsites.net/api");
    REQUIRE(RestHelper::GetRestAPIBaseUri("http://rest_sourc e.azurewebsites.net/api") == L"http://rest_sourc%20e.azurewebsites.net/api");
    REQUIRE(RestHelper::GetRestAPIBaseUri("http://restsource.azurewebsites.net/v1.0/%v1") == L"http://restsource.azurewebsites.net/v1.0/%25v1");
}

TEST_CASE("IsValidUri", "[RestSource]")
{
    REQUIRE(RestHelper::IsValidUri(L"http://rest%20source.azurewebsites.net/api"));
    REQUIRE(!RestHelper::IsValidUri(L"http://rest source.azurewebsites.net/api"));
}

TEST_CASE("AppendPathToUri", "[RestSource]")
{
    REQUIRE(RestHelper::AppendPathToUri(L"http://restsource.azurewebsites.net/api/", L"/path") == L"http://restsource.azurewebsites.net/api/path");
    REQUIRE(RestHelper::AppendPathToUri(L"http://restsource.azurewebsites.net/api/", L"/pat  h") == L"http://restsource.azurewebsites.net/api/pat%20%20h");
    REQUIRE(RestHelper::AppendPathToUri(L"http://restsource.azurewebsites.net/api/", L"/path+version") == L"http://restsource.azurewebsites.net/api/path%2Bversion");
}

TEST_CASE("AppendQueryParamsToUri", "[RestSource]")
{
    utility::string_t url = L"http://restsource.azurewebsites.net/api";
    std::map<std::string_view, std::string> queryParams;
    queryParams.emplace("Version", "1.0 .0");
    queryParams.emplace("Channel", "beta+");

    REQUIRE(RestHelper::AppendQueryParamsToUri(url, queryParams) == L"http://restsource.azurewebsites.net/api?Channel=beta%2B&Version=1.0%20.0");
}

TEST_CASE("GetUniqueItems", "[RestSource]")
{
    std::vector<std::string> list;
    REQUIRE(RestHelper::GetUniqueItems(list).size() == 0);

    std::vector<std::string> listWithDuplicates;
    listWithDuplicates.emplace_back("object1");
    listWithDuplicates.emplace_back("object1");
    listWithDuplicates.emplace_back("object2");
    listWithDuplicates.emplace_back("object2");
    listWithDuplicates.emplace_back("object3");
    std::vector<std::string> result = RestHelper::GetUniqueItems(listWithDuplicates);
    REQUIRE(result.size() == 3);
    REQUIRE(result.at(0) == "object1");
    REQUIRE(result.at(1) == "object2");
    REQUIRE(result.at(2) == "object3");
}
