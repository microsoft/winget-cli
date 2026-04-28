// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"
#include "TestSource.h"
#include "AppInstallerStrings.h"
#include "MatchCriteriaResolver.h"

using namespace AppInstaller;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility;
using namespace TestCommon;


void RequireMatchCriteria(const PackageMatchFilter& expected, const PackageMatchFilter& actual)
{
    REQUIRE(expected.Field == actual.Field);
    REQUIRE(expected.Type == actual.Type);
    REQUIRE(expected.Value == actual.Value);
}

TEST_CASE("MatchCriteriaResolver_MatchType", "[MatchCriteriaResolver]")
{
    Manifest::Manifest manifest;
    PackageMatchFilter expected{ PackageMatchField::Id, MatchType::Wildcard, "Not set by test" };
    std::string searchString = "Search";

    SECTION("Exact")
    {
        manifest.Id = searchString;
        expected.Type = MatchType::Exact;
    }
    SECTION("Case Insensitive")
    {
        manifest.Id = "search";
        expected.Type = MatchType::CaseInsensitive;
    }
    SECTION("Starts With")
    {
        manifest.Id = "Search Result";
        expected.Type = MatchType::StartsWith;
    }
    SECTION("Substring")
    {
        manifest.Id = "Contains searches within";
        expected.Type = MatchType::Substring;
    }
    SECTION("None")
    {
        expected.Field = PackageMatchField::Unknown;
    }

    expected.Value = manifest.Id;

    SearchRequest request;
    request.Query = RequestMatch{ MatchType::Substring, searchString };

    TestPackageVersion packageVersion(manifest);

    PackageMatchFilter actual = FindBestMatchCriteria(request, &packageVersion);
    RequireMatchCriteria(expected, actual);
}

TEST_CASE("MatchCriteriaResolver_MatchField", "[MatchCriteriaResolver]")
{
    Manifest::Manifest manifest;
    Utility::NormalizedString searchString = "Search";
    auto foldedSearchString = Utility::FoldCase(searchString);
    PackageMatchFilter expected{ PackageMatchField::Unknown, MatchType::Exact, searchString };

    SECTION("Identifier")
    {
        manifest.Id = searchString;
        expected.Field = PackageMatchField::Id;
    }
    SECTION("Name")
    {
        manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>(searchString);
        expected.Field = PackageMatchField::Name;
    }
    SECTION("Moniker")
    {
        manifest.Moniker = searchString;
        expected.Field = PackageMatchField::Moniker;
    }
    SECTION("Command")
    {
        manifest.Installers.emplace_back().Commands.emplace_back(searchString);
        expected.Field = PackageMatchField::Command;
    }
    SECTION("Tag")
    {
        manifest.DefaultLocalization.Add<Manifest::Localization::Tags>({ searchString });
        expected.Field = PackageMatchField::Tag;
    }
    SECTION("Package Family Name")
    {
        manifest.Installers.emplace_back().PackageFamilyName = searchString;
        expected.Field = PackageMatchField::PackageFamilyName;
        // Folded by test package version
        expected.Type = MatchType::CaseInsensitive;
        expected.Value = foldedSearchString;
    }
    SECTION("Product Code")
    {
        manifest.Installers.emplace_back().ProductCode = searchString;
        expected.Field = PackageMatchField::ProductCode;
        // Folded by test package version
        expected.Type = MatchType::CaseInsensitive;
        expected.Value = foldedSearchString;
    }
    SECTION("Upgrade Code")
    {
        manifest.Installers.emplace_back().AppsAndFeaturesEntries.emplace_back().UpgradeCode = searchString;
        expected.Field = PackageMatchField::UpgradeCode;
        // Folded by test package version
        expected.Type = MatchType::CaseInsensitive;
        expected.Value = foldedSearchString;
    }

    SearchRequest request;
    request.Query = RequestMatch{ MatchType::Substring, searchString };

    TestPackageVersion packageVersion(manifest);

    PackageMatchFilter actual = FindBestMatchCriteria(request, &packageVersion);
    RequireMatchCriteria(expected, actual);
}

TEST_CASE("MatchCriteriaResolver_Complex", "[MatchCriteriaResolver]")
{
    Manifest::Manifest manifest;
    Utility::NormalizedString searchString = "Search";
    auto foldedSearchString = Utility::FoldCase(searchString);
    PackageMatchFilter expected{ PackageMatchField::Tag, MatchType::Exact, searchString };

    manifest.Id = "Identifer search substring";
    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>("Search name starts");
    manifest.Moniker = foldedSearchString;
    manifest.Installers.emplace_back().Commands.emplace_back("Command search string");
    manifest.DefaultLocalization.Add<Manifest::Localization::Tags>({ searchString });
    manifest.Installers.emplace_back().PackageFamilyName = searchString;
    manifest.Installers.emplace_back().ProductCode = searchString;
    manifest.Installers.emplace_back().AppsAndFeaturesEntries.emplace_back().UpgradeCode = searchString;

    SearchRequest request;
    request.Query = RequestMatch{ MatchType::Substring, searchString };

    TestPackageVersion packageVersion(manifest);

    PackageMatchFilter actual = FindBestMatchCriteria(request, &packageVersion);
    RequireMatchCriteria(expected, actual);
}
