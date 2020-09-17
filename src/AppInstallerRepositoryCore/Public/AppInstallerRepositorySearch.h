// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>
#include <AppInstallerVersions.h>
#include <winget/LocIndependent.h>
#include <winget/Manifest.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::Repository
{
    // The type of matching to perform during a search.
    // The values must be declared in order of preference in search results.
    enum class MatchType
    {
        Exact = 0,
        CaseInsensitive,
        StartsWith,
        Fuzzy,
        Substring,
        FuzzySubstring,
        Wildcard,
    };

    // The field to match on.
    // The values must be declared in order of preference in search results.
    enum class PackageMatchField
    {
        Id = 0,
        Name,
        Moniker,
        Command,
        Tag,
        PackageFamilyName,
        ProductCode,
    };

    // A single match to be performed during a search.
    struct RequestMatch
    {
        MatchType Type;
        Utility::NormalizedString Value;

        RequestMatch(MatchType t, std::string_view v) : Type(t), Value(v) {}
    };

    // A match on a specific field to be performed during a search.
    struct PackageMatchFilter : public RequestMatch
    {
        PackageMatchField Field;

        PackageMatchFilter(PackageMatchField f, MatchType t, std::string_view v) : RequestMatch(t, v), Field(f) {}
    };

    // Container for data used to filter the available manifests in a source.
    // It can be thought of as:
    //  (Query || Inclusions...) && Filters...
    // If Query and Inclusions are both empty, the starting data set will be the entire database.
    //  Everything && Filters...
    struct SearchRequest
    {
        // The generic query matches against a source defined set of fields.
        std::optional<RequestMatch> Query;

        // Specific fields used to include more data.
        // If Query is defined, this can add more rows afterward.
        // If Query is not defined, this is the only set of data included.
        std::vector<PackageMatchFilter> Inclusions;

        // Specific fields used to filter the data further.
        std::vector<PackageMatchFilter> Filters;

        // The maximum number of results to return.
        // The default of 0 will place no limit.
        size_t MaximumResults{};

        // Returns a string summarizing the search request.
        std::string ToString() const;
    };

    // A property of a package.
    enum class PackageProperty
    {
        Id,
        Name,
        SourceId,
        Version,
        Channel,
    };

    // A single package version.
    struct IPackageVersion
    {
        virtual ~IPackageVersion() = default;

        // Gets a property of this package version.
        virtual Utility::LocIndString GetProperty(PackageProperty property) const = 0;

        // Gets the manifest of this package version.
        virtual std::optional<Manifest::Manifest> GetManifest() const = 0;
    };

    // A key to identify a package version within a package.
    struct PackageVersionKey
    {
        // The source id that this version came from.
        Utility::NormalizedString SourceId;

        // The version.
        Utility::NormalizedString Version;

        // The channel.
        Utility::NormalizedString Channel;
    };

    // A package, potentially containing information about it's local state and the available versions.
    struct IPackage
    {
        virtual ~IPackage() = default;

        // Gets the installed package information.
        virtual std::shared_ptr<IPackageVersion> GetInstalledVersion() const = 0;

        // Gets all available versions of this package.
        // The versions will be returned in sorted, descending order.
        //  Ex. { 4, 3, 2, 1 }
        virtual std::vector<PackageVersionKey> GetAvailableVersionKeys() const = 0;

        // Gets a specific version of this package.
        virtual std::shared_ptr<IPackageVersion> GetLatestAvailableVersion() = 0;

        // Gets a specific version of this package.
        virtual std::shared_ptr<IPackageVersion> GetAvailableVersion(const PackageVersionKey& versionKey) = 0;
    };

    // A single result from the search.
    struct ResultMatch
    {
        // The package found by the search request.
        std::unique_ptr<IPackage> Package;

        // The highest order field on which the package matched the search.
        PackageMatchFilter MatchCriteria;

        // The name of the source where the result is from. Used in aggregated source scenario.
        std::string SourceName = {};

        ResultMatch(std::unique_ptr<IPackage>&& p, PackageMatchFilter f) : Package(std::move(p)), MatchCriteria(std::move(f)) {}
    };

    // Search result data.
    struct SearchResult
    {
        // The full set of results from the search.
        std::vector<ResultMatch> Matches;

        // If true, the results were truncated by the given SearchRequest::MaximumResults.
        bool Truncated = false;
    };

    inline std::string_view MatchTypeToString(MatchType type)
    {
        using namespace std::string_view_literals;

        switch (type)
        {
        case MatchType::Exact:
            return "Exact"sv;
        case MatchType::CaseInsensitive:
            return "CaseInsensitive"sv;
        case MatchType::StartsWith:
            return "StartsWith"sv;
        case MatchType::Substring:
            return "Substring"sv;
        case MatchType::Wildcard:
            return "Wildcard"sv;
        case MatchType::Fuzzy:
            return "Fuzzy"sv;
        case MatchType::FuzzySubstring:
            return "FuzzySubstring"sv;
        }

        return "UnknownMatchType"sv;
    }

    inline std::string_view PackageMatchFieldToString(PackageMatchField matchField)
    {
        using namespace std::string_view_literals;

        switch (matchField)
        {
        case PackageMatchField::Command:
            return "Command"sv;
        case PackageMatchField::Id:
            return "Id"sv;
        case PackageMatchField::Moniker:
            return "Moniker"sv;
        case PackageMatchField::Name:
            return "Name"sv;
        case PackageMatchField::Tag:
            return "Tag"sv;
        case ApplicationMatchField::PackageFamilyName:
            return "PackageFamilyName"sv;
        case ApplicationMatchField::ProductCode:
            return "ProductCode"sv;
        }

        return "UnknownMatchField"sv;
    }
}
