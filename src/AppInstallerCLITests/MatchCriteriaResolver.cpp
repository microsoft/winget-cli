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

TEST_CASE("MatchCriteriaResolver_ResolveUnknownCriteria", "[MatchCriteriaResolver]")
{
    Manifest::Manifest manifest;
    manifest.Id = "Foo.Bar";
    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>("Foo Bar");
    manifest.Moniker = "foobar";

    const PackageMatchFilter idMatch{ PackageMatchField::Id, MatchType::Exact, "Foo.Bar" };
    const PackageMatchFilter idMismatch{ PackageMatchField::Id, MatchType::Exact, "Other.Package" };
    const PackageMatchFilter nameMatch{ PackageMatchField::Name, MatchType::Exact, "Foo Bar" };
    const PackageMatchFilter monikerMismatch{ PackageMatchField::Moniker, MatchType::Exact, "other" };
    const PackageMatchFilter unknown{ PackageMatchField::Market, MatchType::Exact, "US" };

    struct MatchCase
    {
        std::string_view Name;
        std::vector<PackageMatchFilter> Filters;
        std::vector<PackageMatchFilter> Inclusions;
        bool HasQuery;
        std::optional<bool> Expected;
        std::vector<PackageMatchField> ResolvedFields;
    };

    const MatchCase cases[] =
    {
        { "Empty request", {}, {}, false, true, {} },
        { "Known matching filter", { idMatch }, {}, false, true, {} },
        { "Known failed filter after unknown", { nameMatch, idMismatch }, {}, false, false, {} },
        { "Known inclusion after unknown", {}, { nameMatch, idMatch }, false, true, {} },
        { "Failed inclusions with unknown filter", { nameMatch }, { idMismatch }, false, false, {} },
        { "Failed filter with unknown inclusion", { idMismatch }, { nameMatch }, false, false, {} },
        { "Resolve matching filter", { idMatch, nameMatch }, {}, false, true, { PackageMatchField::Name } },
        { "Resolve failing filter", { monikerMismatch }, {}, false, false, { PackageMatchField::Moniker } },
        { "Filter remains unknown", { unknown }, {}, false, std::nullopt, { PackageMatchField::Market } },
        { "Resolve matching inclusion", {}, { idMismatch, nameMatch }, false, true, { PackageMatchField::Name } },
        { "Resolve failing inclusion", {}, { monikerMismatch }, false, false, { PackageMatchField::Moniker } },
        { "Inclusion remains unknown", {}, { unknown }, false, std::nullopt, { PackageMatchField::Market } },
        { "Resolved mismatch stops later filters", { monikerMismatch, nameMatch }, {}, false, false, { PackageMatchField::Moniker } },
        { "Resolved mismatch avoids inclusion lookup", { monikerMismatch }, { nameMatch }, false, false, { PackageMatchField::Moniker } },
        { "Resolved match stops later inclusions", {}, { nameMatch, monikerMismatch }, false, true, { PackageMatchField::Name } },
        { "Resolve next inclusion after mismatch", {}, { monikerMismatch, nameMatch }, false, true, { PackageMatchField::Moniker, PackageMatchField::Name } },
        { "Unknown filter cannot override failed inclusions", { unknown }, { monikerMismatch }, false, false, { PackageMatchField::Market, PackageMatchField::Moniker } },
        { "Matching inclusion cannot prove unknown filter", { unknown }, { nameMatch }, false, std::nullopt, { PackageMatchField::Market, PackageMatchField::Name } },
        { "Matching filter cannot prove unknown inclusion", { nameMatch }, { unknown }, false, std::nullopt, { PackageMatchField::Name, PackageMatchField::Market } },
        { "Source-defined query", {}, {}, true, std::nullopt, {} },
        { "Query makes inclusion lookup unnecessary", {}, { nameMatch }, true, std::nullopt, {} },
        { "Query still requires filter resolution", { nameMatch }, { monikerMismatch }, true, std::nullopt, { PackageMatchField::Name } },
        { "Query cannot override resolved filter failure", { monikerMismatch }, { nameMatch }, true, false, { PackageMatchField::Moniker } },
        { "Known inclusion alongside query", {}, { nameMatch, idMatch }, true, true, {} },
        { "Resolve only filter when selection is known", { unknown }, { nameMatch, idMatch }, false, std::nullopt, { PackageMatchField::Market } },
    };

    auto matchesField = [&](const PackageMatchFilter& field) -> std::optional<bool>
    {
        return field.Field == PackageMatchField::Id ? MatchesRequest(field, manifest.Id) : std::nullopt;
    };
    for (const auto& test : cases)
    {
        CAPTURE(test.Name);
        SearchRequest request;
        request.Filters = test.Filters;
        request.Inclusions = test.Inclusions;
        if (test.HasQuery)
        {
            request.Query.emplace(MatchType::Substring, "Source-defined query");
        }

        std::vector<PackageMatchField> resolvedFields;
        auto resolveField = [&](const PackageMatchFilter& field)
        {
            resolvedFields.emplace_back(field.Field);
            return MatchesRequest(field, manifest);
        };
        REQUIRE(MatchesRequest(request, matchesField, resolveField) == test.Expected);
        REQUIRE(resolvedFields == test.ResolvedFields);
    }
}

TEST_CASE("MatchCriteriaResolver_ResolutionReusesAvailableMetadata", "[MatchCriteriaResolver]")
{
    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Name, MatchType::Exact, "Foo Bar");
    request.Filters.emplace_back(PackageMatchField::Tag, MatchType::Exact, "utility");
    request.Inclusions.emplace_back(PackageMatchField::Moniker, MatchType::Exact, "foobar");

    std::optional<Manifest::Manifest> manifest;
    auto matchesField = [&](const PackageMatchFilter& field) -> std::optional<bool>
    {
        return manifest ? MatchesRequest(field, manifest.value()) : std::nullopt;
    };
    std::vector<PackageMatchField> resolvedFields;
    auto resolveField = [&](const PackageMatchFilter& field)
    {
        resolvedFields.emplace_back(field.Field);
        auto& data = manifest.emplace();
        data.Id = "Foo.Bar";
        data.DefaultLocalization.Add<Manifest::Localization::PackageName>("Foo Bar");
        data.DefaultLocalization.Add<Manifest::Localization::Tags>({ "utility" });
        data.Moniker = "foobar";
        return MatchesRequest(field, data);
    };

    REQUIRE(MatchesRequest(request, matchesField, resolveField) == std::optional<bool>{ true });
    REQUIRE(resolvedFields == std::vector<PackageMatchField>{ PackageMatchField::Name });
}

TEST_CASE("MatchCriteriaResolver_ResolutionRefreshesEarlierCriteria", "[MatchCriteriaResolver]")
{
    const PackageMatchFilter nameMatch{ PackageMatchField::Name, MatchType::Exact, "Foo Bar" };
    const PackageMatchFilter nameMismatch{ PackageMatchField::Name, MatchType::Exact, "Other" };
    const PackageMatchFilter monikerMatch{ PackageMatchField::Moniker, MatchType::Exact, "foobar" };
    const PackageMatchFilter monikerMismatch{ PackageMatchField::Moniker, MatchType::Exact, "other" };
    const PackageMatchFilter tagMatch{ PackageMatchField::Tag, MatchType::Exact, "utility" };
    SearchRequest request;
    request.Filters = { nameMismatch };
    request.Inclusions = { monikerMatch };
    std::optional<bool> expected = false;
    std::vector<PackageMatchField> expectedResolvedFields{ PackageMatchField::Name, PackageMatchField::Moniker };
    bool tagKnownOnlyToResolver = false;

    SECTION("Inclusion resolution reveals a failed filter") {}
    SECTION("Inclusion resolution confirms an earlier filter")
    {
        request.Filters = { nameMatch };
        expected = true;
    }
    SECTION("Later filter resolution reveals an earlier failure")
    {
        request.Filters = { nameMismatch, tagMatch };
        request.Filters.emplace_back(PackageMatchField::Market, MatchType::Exact, "US");
        expectedResolvedFields = { PackageMatchField::Name, PackageMatchField::Tag };
    }
    SECTION("Later inclusion resolution reveals an earlier match")
    {
        request.Filters.clear();
        request.Inclusions = { nameMatch, monikerMismatch };
        expected = true;
    }
    SECTION("Later inclusion resolution rules out all alternatives")
    {
        request.Filters.clear();
        request.Inclusions = { nameMismatch, monikerMismatch };
    }
    SECTION("Refreshing unknowns preserves definitive resolver answers")
    {
        request.Filters = { tagMatch, nameMatch };
        tagKnownOnlyToResolver = true;
        expected = true;
        expectedResolvedFields = { PackageMatchField::Tag, PackageMatchField::Name, PackageMatchField::Moniker };
    }

    std::optional<Manifest::Manifest> manifest;
    auto matchesField = [&](const PackageMatchFilter& field) -> std::optional<bool>
    {
        if (tagKnownOnlyToResolver && field.Field == PackageMatchField::Tag)
        {
            return std::nullopt;
        }
        return manifest ? MatchesRequest(field, manifest.value()) : std::nullopt;
    };
    std::vector<PackageMatchField> resolvedFields;
    auto resolveField = [&](const PackageMatchFilter& field) -> std::optional<bool>
    {
        resolvedFields.emplace_back(field.Field);
        if (field.Field == PackageMatchField::Name)
        {
            return std::nullopt;
        }
        if (tagKnownOnlyToResolver && field.Field == PackageMatchField::Tag)
        {
            return true;
        }
        auto& data = manifest.emplace();
        data.Id = "Foo.Bar";
        data.DefaultLocalization.Add<Manifest::Localization::PackageName>("Foo Bar");
        data.DefaultLocalization.Add<Manifest::Localization::Tags>({ "utility" });
        data.Moniker = "foobar";
        return MatchesRequest(field, data);
    };

    REQUIRE(MatchesRequest(request, matchesField, resolveField) == expected);
    REQUIRE(resolvedFields == expectedResolvedFields);
}

TEST_CASE("MatchCriteriaResolver_ResolutionFailure", "[MatchCriteriaResolver]")
{
    SearchRequest request;
    request.Filters.emplace_back(PackageMatchField::Name, MatchType::Exact, "Foo Bar");
    auto matchesField = [](const PackageMatchFilter&) -> std::optional<bool>
    {
        return std::nullopt;
    };
    auto resolveField = [](const PackageMatchFilter&) -> std::optional<bool>
    {
        THROW_HR(E_ACCESSDENIED);
    };

    REQUIRE_THROWS_HR(MatchesRequest(request, matchesField, resolveField), E_ACCESSDENIED);
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
