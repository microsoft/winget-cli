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

TEST_CASE("MatchCriteriaResolver_MatchesRequest", "[MatchCriteriaResolver]")
{
    struct MatchCase
    {
        MatchType Type;
        std::string_view Query;
        std::string_view Value;
        bool Expected;
    };

    const MatchCase cases[] =
    {
        { MatchType::Exact, "Foo.Bar", "Foo.Bar", true },
        { MatchType::Exact, "foo.bar", "Foo.Bar", false },
        { MatchType::Exact, "Foo", "Foo.Bar", false },
        { MatchType::CaseInsensitive, "foo.bar", "Foo.Bar", true },
        { MatchType::CaseInsensitive, "foo", "Foo.Bar", false },
        { MatchType::StartsWith, "foo", "Foo.Bar", true },
        { MatchType::StartsWith, "bar", "Foo.Bar", false },
        { MatchType::Substring, "BAR", "Foo.Bar", true },
        { MatchType::Substring, "Baz", "Foo.Bar", false },
        { MatchType::Exact, "caf\xC3\xA9", "cafe\xCC\x81", true },
        { MatchType::CaseInsensitive, "CAF\xC3\x89", "caf\xC3\xA9", true },
        { MatchType::Exact, "", "Foo.Bar", false },
    };

    for (const auto& test : cases)
    {
        CAPTURE(ToString(test.Type), test.Query, test.Value);
        auto result = MatchesRequest(RequestMatch{ test.Type, test.Query }, test.Value);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == test.Expected);
    }
}

TEST_CASE("MatchCriteriaResolver_MatchesRequest_Unsupported", "[MatchCriteriaResolver]")
{
    auto type = GENERATE(MatchType::Fuzzy, MatchType::FuzzySubstring, MatchType::Wildcard);
    REQUIRE_FALSE(MatchesRequest(RequestMatch{ type, "Foo" }, "Foo.Bar").has_value());
}

TEST_CASE("MatchCriteriaResolver_ManifestFields", "[MatchCriteriaResolver]")
{
    Manifest::Manifest manifest;
    manifest.Id = "Foo.Bar";
    manifest.Moniker = "FooBar";
    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>("Foo Bar");
    manifest.DefaultLocalization.Add<Manifest::Localization::Tags>({ "DefaultTag" });
    auto& localization = manifest.Localizations.emplace_back();
    localization.Add<Manifest::Localization::PackageName>("Localized Name");
    localization.Add<Manifest::Localization::Tags>({ "LocalizedTag" });
    manifest.Localizations.emplace_back().Add<Manifest::Localization::PackageName>(u8"Caf\u00E9");
    auto& installer = manifest.Installers.emplace_back();
    installer.Commands.emplace_back("ToolCmd");
    installer.PackageFamilyName = "Foo.Bar_123";
    installer.ProductCode = "Installer.Code";
    auto& entry = installer.AppsAndFeaturesEntries.emplace_back();
    entry.DisplayName = "Installed Name";
    entry.ProductCode = "ARP.Code";
    entry.UpgradeCode = "ARP.Upgrade";

    struct MatchCase
    {
        PackageMatchField Field;
        MatchType Type;
        std::string_view Value;
        std::optional<bool> Expected;
    };

    const MatchCase cases[] =
    {
        { PackageMatchField::Id, MatchType::Exact, "Foo.Bar", true },
        { PackageMatchField::Id, MatchType::Exact, "foo.bar", false },
        { PackageMatchField::Name, MatchType::Exact, "Foo Bar", true },
        { PackageMatchField::Name, MatchType::Exact, "foo bar", false },
        { PackageMatchField::Name, MatchType::CaseInsensitive, "foo bar", true },
        { PackageMatchField::Name, MatchType::Exact, "Localized Name", true },
        { PackageMatchField::Name, MatchType::Exact, "localized name", false },
        { PackageMatchField::Name, MatchType::Exact, "Installed Name", true },
        { PackageMatchField::Name, MatchType::Exact, "installed name", false },
        { PackageMatchField::Name, MatchType::StartsWith, "localized", true },
        { PackageMatchField::Name, MatchType::Substring, "NAME", true },
        { PackageMatchField::Name, MatchType::Exact, u8"Cafe\u0301", true },
        { PackageMatchField::Moniker, MatchType::Exact, "FooBar", true },
        { PackageMatchField::Moniker, MatchType::Exact, "foobar", false },
        { PackageMatchField::Moniker, MatchType::CaseInsensitive, "foobar", true },
        { PackageMatchField::Tag, MatchType::Exact, "DefaultTag", true },
        { PackageMatchField::Tag, MatchType::Exact, "LocalizedTag", true },
        { PackageMatchField::Tag, MatchType::Exact, "localizedtag", false },
        { PackageMatchField::Command, MatchType::Exact, "ToolCmd", true },
        { PackageMatchField::Command, MatchType::Exact, "toolcmd", false },
        { PackageMatchField::PackageFamilyName, MatchType::Exact, "FOO.BAR_123", true },
        { PackageMatchField::ProductCode, MatchType::Exact, "INSTALLER.CODE", true },
        { PackageMatchField::ProductCode, MatchType::Exact, "ARP.CODE", true },
        { PackageMatchField::UpgradeCode, MatchType::Exact, "ARP.UPGRADE", true },
        { PackageMatchField::Name, MatchType::Fuzzy, "Foo", std::nullopt },
        { PackageMatchField::Name, MatchType::FuzzySubstring, "Foo", std::nullopt },
        { PackageMatchField::Name, MatchType::Wildcard, "Foo*", std::nullopt },
        { PackageMatchField::NormalizedNameAndPublisher, MatchType::Exact, "Foo Bar", std::nullopt },
        { PackageMatchField::Market, MatchType::Exact, "US", std::nullopt },
        { PackageMatchField::Unknown, MatchType::Exact, "Foo Bar", std::nullopt },
    };

    for (const auto& test : cases)
    {
        CAPTURE(ToString(test.Field), ToString(test.Type), test.Value);
        REQUIRE(MatchesRequest(PackageMatchFilter{ test.Field, test.Type, test.Value }, manifest) == test.Expected);
    }
    for (auto field : { PackageMatchField::Id, PackageMatchField::Name, PackageMatchField::Moniker,
        PackageMatchField::Tag, PackageMatchField::Command, PackageMatchField::PackageFamilyName,
        PackageMatchField::ProductCode, PackageMatchField::UpgradeCode })
    {
        CAPTURE(ToString(field));
        REQUIRE(MatchesRequest(PackageMatchFilter{ field, MatchType::Exact, "Missing.Value" }, manifest) == std::optional<bool>{ false });
    }
}

TEST_CASE("MatchCriteriaResolver_ManifestEmptyFields", "[MatchCriteriaResolver]")
{
    Manifest::Manifest manifest;
    auto field = GENERATE(PackageMatchField::Moniker, PackageMatchField::Tag, PackageMatchField::Command,
        PackageMatchField::PackageFamilyName, PackageMatchField::ProductCode, PackageMatchField::UpgradeCode);
    auto type = GENERATE(MatchType::Exact, MatchType::CaseInsensitive, MatchType::StartsWith, MatchType::Substring);
    CAPTURE(ToString(field), ToString(type));
    PackageMatchFilter request{ field, type, "" };
    REQUIRE(MatchesRequest(request, manifest) == std::optional<bool>{ false });
    request.Type = MatchType::Wildcard;
    REQUIRE_FALSE(MatchesRequest(request, manifest).has_value());
}

TEST_CASE("MatchCriteriaResolver_SearchRequest", "[MatchCriteriaResolver]")
{
    const PackageMatchFilter idMatch{ PackageMatchField::Id, MatchType::CaseInsensitive, "microsoft.powertoys" };
    const PackageMatchFilter nameMatch{ PackageMatchField::Name, MatchType::Exact, "Microsoft PowerToys" };
    const PackageMatchFilter idMismatch{ PackageMatchField::Id, MatchType::Exact, "Other.Package" };
    const PackageMatchFilter unknown{ PackageMatchField::Moniker, MatchType::CaseInsensitive, "powertoys" };
    const PackageMatchFilter unsupported{ PackageMatchField::Id, MatchType::Fuzzy, "powertoys" };

    struct MatchCase
    {
        std::string_view Name;
        std::vector<PackageMatchFilter> Filters;
        std::vector<PackageMatchFilter> Inclusions;
        bool HasQuery;
        std::optional<bool> Expected;
    };

    const MatchCase cases[] =
    {
        { "Empty request", {}, {}, false, true },
        { "All filters match", { idMatch, nameMatch }, {}, false, true },
        { "Every filter must match", { idMatch, idMismatch }, {}, false, false },
        { "Unknown filter", { idMatch, unknown }, {}, false, std::nullopt },
        { "Failed filter after unknown", { unknown, idMismatch }, {}, false, false },
        { "Any inclusion may match", {}, { idMismatch, nameMatch }, false, true },
        { "Failed inclusions", {}, { idMismatch }, false, false },
        { "Unknown inclusion may match", {}, { idMismatch, unknown }, false, std::nullopt },
        { "Match after unknown inclusion", {}, { unknown, idMatch }, false, true },
        { "Inclusion cannot override failed filter", { idMismatch }, { nameMatch }, false, false },
        { "Filters cannot override failed inclusions", { idMatch }, { idMismatch }, false, false },
        { "Matching inclusion with unknown filter", { unknown }, { idMatch }, false, std::nullopt },
        { "Failed inclusions with unknown filter", { unknown }, { idMismatch }, false, false },
        { "Failed filter with unknown inclusion", { idMismatch }, { unknown }, false, false },
        { "Unknown filter and inclusion", { unknown }, { unknown }, false, std::nullopt },
        { "Unsupported filter match type", { unsupported }, {}, false, std::nullopt },
        { "Unsupported inclusion match type", {}, { unsupported }, false, std::nullopt },
        { "Source-defined query", {}, {}, true, std::nullopt },
        { "Query may select despite failed inclusions", { idMatch }, { idMismatch }, true, std::nullopt },
        { "Query cannot override failed filter", { idMismatch }, { idMatch }, true, false },
        { "Matching inclusion alongside query", {}, { idMatch }, true, true },
    };

    auto matchesField = [](const PackageMatchFilter& filter) -> std::optional<bool>
    {
        switch (filter.Field)
        {
        case PackageMatchField::Id:
            return MatchesRequest(filter, "Microsoft.PowerToys");
        case PackageMatchField::Name:
            return MatchesRequest(filter, "Microsoft PowerToys");
        default:
            return std::nullopt;
        }
    };

    for (const auto& test : cases)
    {
        CAPTURE(test.Name);
        SearchRequest request;
        request.Filters = test.Filters;
        request.Inclusions = test.Inclusions;
        if (test.HasQuery)
        {
            request.Query.emplace(MatchType::CaseInsensitive, "powertoys");
        }
        REQUIRE(MatchesRequest(request, matchesField) == test.Expected);
    }
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
