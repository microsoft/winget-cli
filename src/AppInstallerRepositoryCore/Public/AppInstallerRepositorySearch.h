// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>
#include <AppInstallerVersions.h>
#include <winget/LocIndependent.h>
#include <winget/Manifest.h>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::Repository
{
    struct ISource;

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
        NormalizedNameAndPublisher,
    };

    // A single match to be performed during a search.
    struct RequestMatch
    {
        MatchType Type;
        Utility::NormalizedString Value;
        std::optional<Utility::NormalizedString> Additional;

        RequestMatch(MatchType t) : Type(t) {}
        RequestMatch(MatchType t, Utility::NormalizedString& v) : Type(t), Value(v) {}
        RequestMatch(MatchType t, const Utility::NormalizedString& v) : Type(t), Value(v) {}
        RequestMatch(MatchType t, Utility::NormalizedString&& v) : Type(t), Value(std::move(v)) {}
        RequestMatch(MatchType t, std::string_view v1, std::string_view v2) : Type(t), Value(v1), Additional(Utility::NormalizedString{ v2 }) {}
    };

    // A match on a specific field to be performed during a search.
    struct PackageMatchFilter : public RequestMatch
    {
        PackageMatchField Field;

        PackageMatchFilter(PackageMatchField f, MatchType t) : RequestMatch(t), Field(f) { EnsureRequiredValues(); }
        PackageMatchFilter(PackageMatchField f, MatchType t, Utility::NormalizedString& v) : RequestMatch(t, v), Field(f) { EnsureRequiredValues(); }
        PackageMatchFilter(PackageMatchField f, MatchType t, const Utility::NormalizedString& v) : RequestMatch(t, v), Field(f) { EnsureRequiredValues(); }
        PackageMatchFilter(PackageMatchField f, MatchType t, Utility::NormalizedString&& v) : RequestMatch(t, std::move(v)), Field(f) { EnsureRequiredValues(); }
        PackageMatchFilter(PackageMatchField f, MatchType t, std::string_view v1, std::string_view v2) : RequestMatch(t, v1, v2), Field(f) { EnsureRequiredValues(); }

    protected:
        void EnsureRequiredValues()
        {
            // Ensure that the second value always exists when it should
            if (Field == PackageMatchField::NormalizedNameAndPublisher && !Additional)
            {
                Additional = Utility::NormalizedString{};
            }
        }
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

        // Returns a value indicating whether this request is for all available data.
        bool IsForEverything() const;

        // Returns a string summarizing the search request.
        std::string ToString() const;
    };

    // A property of a package version.
    enum class PackageVersionProperty
    {
        Id,
        Name,
        SourceIdentifier,
        SourceName,
        Version,
        Channel,
        RelativePath,
        // Returned in hexadecimal format
        ManifestSHA256Hash,
    };

    // A property of a package version that can have multiple values.
    enum class PackageVersionMultiProperty
    {
        // The package family names (PFN) associated with the package version
        PackageFamilyName,
        // The product codes associated with the package version.
        ProductCode,
        // TODO: Fully implement these 3; the data is not yet in the index source (name and publisher are hacks and locale is not present)
        // The package names for the version; these must match in number and order with both Publisher and Locale.
        Name,
        // The publisher values for the version; these must match in number and order with both Name and Locale.
        Publisher,
        // The locale of the matching Name and Publisher values; these must match in number and order with both Name and Publisher.
        // May be empty if there is only a single value for Name and Publisher.
        Locale,
    };

    // A metadata item of a package version.
    enum class PackageVersionMetadata : int32_t
    {
        // The InstallerType of an installed package
        InstalledType,
        // The Scope of an installed package
        InstalledScope,
        // The system path where the package is installed
        InstalledLocation,
        // The standard uninstall command; which may be interactive
        StandardUninstallCommand,
        // An uninstall command that should be non-interactive
        SilentUninstallCommand,
        // The publisher of the package
        Publisher,
        // The locale of the package
        InstalledLocale,
    };

    // Convert a PackageVersionMetadata to a string.
    std::string_view ToString(PackageVersionMetadata pvm);

    // A single package version.
    struct IPackageVersion
    {
        using Metadata = std::map<PackageVersionMetadata, std::string>;

        virtual ~IPackageVersion() = default;

        // Gets a property of this package version.
        virtual Utility::LocIndString GetProperty(PackageVersionProperty property) const = 0;

        // Gets a property of this package version that can have multiple values.
        virtual std::vector<Utility::LocIndString> GetMultiProperty(PackageVersionMultiProperty property) const = 0;

        // Gets the manifest of this package version.
        virtual Manifest::Manifest GetManifest() = 0;

        // Gets the source where this package version is from.
        virtual std::shared_ptr<const ISource> GetSource() const = 0;

        // Gets any metadata associated with this package version.
        // Primarily stores data on installed packages.
        virtual Metadata GetMetadata() const = 0;
    };

    // An installed package version.
    struct IInstalledPackageVersion : public IPackageVersion
    {
        // Sets metadata on the installed version.
        virtual void SetMetadata(PackageVersionMetadata metadata, std::string_view value) = 0;
    };

    // A key to identify a package version within a package.
    struct PackageVersionKey
    {
        PackageVersionKey() = default;

        PackageVersionKey(Utility::NormalizedString sourceId, Utility::NormalizedString version, Utility::NormalizedString channel) :
            SourceId(std::move(sourceId)), Version(std::move(version)), Channel(std::move(channel)) {}

        // The source id that this version came from.
        std::string SourceId;

        // The version.
        Utility::NormalizedString Version;

        // The channel.
        Utility::NormalizedString Channel;
    };

    // A property of a package.
    enum class PackageProperty
    {
        Id,
        Name,
    };

    // A package, potentially containing information about it's local state and the available versions.
    struct IPackage
    {
        virtual ~IPackage() = default;

        // Gets a property of this package.
        virtual Utility::LocIndString GetProperty(PackageProperty property) const = 0;

        // Gets the installed package information.
        virtual std::shared_ptr<IPackageVersion> GetInstalledVersion() const = 0;

        // Gets all available versions of this package.
        // The versions will be returned in sorted, descending order.
        //  Ex. { 4, 3, 2, 1 }
        virtual std::vector<PackageVersionKey> GetAvailableVersionKeys() const = 0;

        // Gets a specific version of this package.
        virtual std::shared_ptr<IPackageVersion> GetLatestAvailableVersion() const = 0;

        // Gets a specific version of this package.
        virtual std::shared_ptr<IPackageVersion> GetAvailableVersion(const PackageVersionKey& versionKey) const = 0;

        // Gets a value indicating whether an available version is newer than the installed version.
        virtual bool IsUpdateAvailable() const = 0;

        // Determines if the given IPackage refers to the same package as this one.
        virtual bool IsSame(const IPackage*) const = 0;
    };

    // A single result from the search.
    struct ResultMatch
    {
        // The package found by the search request.
        std::shared_ptr<IPackage> Package;

        // The highest order field on which the package matched the search.
        PackageMatchFilter MatchCriteria;

        ResultMatch(std::shared_ptr<IPackage> p, PackageMatchFilter f) : Package(std::move(p)), MatchCriteria(std::move(f)) {}
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
        case PackageMatchField::PackageFamilyName:
            return "PackageFamilyName"sv;
        case PackageMatchField::ProductCode:
            return "ProductCode"sv;
        case PackageMatchField::NormalizedNameAndPublisher:
            return "NormalizedNameAndPublisher"sv;
        }

        return "UnknownMatchField"sv;
    }
}
