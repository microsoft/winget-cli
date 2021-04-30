// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestRestRequestHandler.h"
#include <AppInstallerErrors.h>
#include <Rest/Schema/1_0/Json/SearchRequestSerializer.h>

using namespace TestCommon;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Rest::Schema::V1_0;
using namespace AppInstaller::Repository::Rest::Schema::V1_0::Json;

TEST_CASE("SearchRequestSerializer_InclusionsFilters", "[RestSource]")
{
    SearchRequest searchRequest;
    searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Id, MatchType::Substring, "Foo.Bar"));
    searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Name, MatchType::Substring, "Foo"));
    searchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Moniker, MatchType::Exact, "FooBar"));
    searchRequest.MaximumResults = 10;

    SearchRequestSerializer serializer;
    web::json::value actual = serializer.Serialize(searchRequest);

    REQUIRE(!actual.is_null());
    REQUIRE(!actual.has_field(L"FetchAllManifests"));
    REQUIRE(actual.at(L"MaximumResults").as_integer() == static_cast<int>(searchRequest.MaximumResults));

    // Inclusions
    web::json::array inclusions = actual.at(L"Inclusions").as_array();
    REQUIRE(inclusions.size() == 2);
    REQUIRE(inclusions.at(0).at(L"PackageMatchField").as_string() == L"PackageIdentifier");
    REQUIRE(inclusions.at(1).at(L"PackageMatchField").as_string() == L"PackageName");
    web::json::value requestMatch = inclusions.at(0).at(L"RequestMatch");
    REQUIRE(!requestMatch.is_null());
    REQUIRE(requestMatch.at(L"KeyWord").as_string() == L"Foo.Bar");
    REQUIRE(requestMatch.at(L"MatchType").as_string() == L"Substring");

    // Filters
    web::json::array filters = actual.at(L"Filters").as_array();
    REQUIRE(filters.size() == 1);
    REQUIRE(filters.at(0).at(L"PackageMatchField").as_string() == L"Moniker");
    web::json::value requestMatchFilter = filters.at(0).at(L"RequestMatch");
    REQUIRE(!requestMatchFilter.is_null());
    REQUIRE(requestMatchFilter.at(L"KeyWord").as_string() == L"FooBar");
    REQUIRE(requestMatchFilter.at(L"MatchType").as_string() == L"Exact");
}

TEST_CASE("SearchRequestSerializer_Query", "[RestSource]")
{
    SearchRequest searchRequest;
    searchRequest.Query = RequestMatch(MatchType::Substring, "Foo.Bar");
    
    SearchRequestSerializer serializer;
    web::json::value actual = serializer.Serialize(std::move(searchRequest));

    REQUIRE(!actual.is_null());
    web::json::value query = actual.at(L"Query");
    REQUIRE(query.at(L"KeyWord").as_string() == L"Foo.Bar");
    REQUIRE(query.at(L"MatchType").as_string() == L"Substring");
}

TEST_CASE("SearchRequestSerializer_FetchAllManifests", "[RestSource]")
{
    SearchRequestSerializer serializer;
    web::json::value actual = serializer.Serialize({});

    REQUIRE(!actual.is_null());
    REQUIRE(actual.at(L"FetchAllManifests").as_bool());
}
