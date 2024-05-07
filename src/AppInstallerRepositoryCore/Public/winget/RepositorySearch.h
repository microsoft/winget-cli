// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerErrors.h>
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
    struct Source;

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

    // Convert a MatchType to a string.
    std::string_view ToString(MatchType type);

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
        UpgradeCode,
        NormalizedNameAndPublisher,
        Market,
        Unknown = 9999
    };

    // Convert a PackageMatchField to a string.
    std::string_view ToString(PackageMatchField matchField);

    // Parse a string to PackageMatchField.
    PackageMatchField StringToPackageMatchField(std::string_view field);

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

    // The search purpose of the search request.
    enum class SearchPurpose
    {
        // Default search purpose.
        Default,
        // The result is used for correlation to an installed package.
        CorrelationToInstalled,
        // The result is used for correlation to an available package.
        CorrelationToAvailable,
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

        // The search purpose of the search request.
        SearchPurpose Purpose = SearchPurpose::Default;

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
        Publisher,
        ArpMinVersion,
        ArpMaxVersion,
        Moniker,
    };

    // A property of a package version that can have multiple values.
    enum class PackageVersionMultiProperty
    {
        // The package family names (PFN) associated with the package version
        PackageFamilyName,
        // The product codes associated with the package version.
        ProductCode,
        // The upgrade codes associated with the package version.
        UpgradeCode,
        // TODO: Fully implement these 3; the data is not yet in the index source (name and publisher are hacks and locale is not present)
        //       For future usage of these, be aware of the limitations.
        // The package names for the version; ideally these would match in number and order with both Publisher and Locale.
        Name,
        // The publisher values for the version; ideally these would match in number and order with both Name and Locale.
        Publisher,
        // The locale of the matching Name and Publisher values; ideally these would match in number and order with both Name and Publisher.
        // May be empty if there is only a single value for Name and Publisher.
        Locale,
        // The tags associated with a package version.
        Tag,
        // The commands associated with a package version.
        Command,
    };

    // A metadata item of a package version. These values are persisted and cannot be changed.
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
        // The write time for the given version
        TrackingWriteTime,
        // The Architecture of an installed package
        InstalledArchitecture,
        // The pinned state of the installed package
        // As a package can have multiple pins for multiple sources, this is the strictest pin
        PinnedState,
        // The Architecture of user intent
        UserIntentArchitecture,
        // The locale of user intent
        UserIntentLocale,
        // The standard modify command; which may be interactive
        StandardModifyCommand,
        // No Modify flag
        NoModify,
        // No Repair flag
        NoRepair,
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
        virtual Source GetSource() const = 0;

        // Gets any metadata associated with this package version.
        // Primarily stores data on installed packages.
        virtual Metadata GetMetadata() const = 0;
    };

    // A key to identify a package version within a package.
    struct PackageVersionKey
    {
        PackageVersionKey() = default;

        PackageVersionKey(std::string sourceId, Utility::NormalizedString version, Utility::NormalizedString channel) :
            SourceId(std::move(sourceId)), Version(std::move(version)), Channel(std::move(channel)) {}

        // The source id that this version came from.
        std::string SourceId;

        // The version.
        Utility::NormalizedString Version;

        // The channel.
        Utility::NormalizedString Channel;

        bool operator<(const PackageVersionKey& other) const
        {
            // Sort using only the version and channel.
            // The order for the sources depends on the context.
            return Utility::VersionAndChannel({ Version }, { Channel }) < Utility::VersionAndChannel({ other.Version }, { other.Channel });
        }

        // Determines if a well defined key (this one) is matched by the provided key.
        // The provided key may use empty values to indicate no specific matching requirements.
        bool IsMatch(const PackageVersionKey& other) const;

        // Determines if this version is the simple "latest" targeting version (version and channel are both empty).
        bool IsDefaultLatest() const;
    };

    // A property of a package.
    enum class PackageProperty
    {
        Id,
        Name,
    };

    // To allow for runtime casting from IPackage to the specific types, this enum contains all of the IPackage implementations.
    enum class IPackageType
    {
        TestPackage,
        RestPackage,
        SQLitePackage1,
        SQLitePackage2,
        PinnablePackage,
        CompositeInstalledPackage,
    };

    // Contains a collection of package versions.
    struct IPackageVersionCollection
    {
        virtual ~IPackageVersionCollection() = default;

        // Gets all versions of this package.
        // The versions will be returned in sorted, descending order.
        //  Ex. { 4, 3, 2, 1 }
        virtual std::vector<PackageVersionKey> GetVersionKeys() const = 0;

        // Gets a specific version of this package.
        virtual std::shared_ptr<IPackageVersion> GetVersion(const PackageVersionKey& versionKey) const = 0;

        // A convenience method to effectively call `GetVersion(GetVersionKeys[0])`.
        virtual std::shared_ptr<IPackageVersion> GetLatestVersion() const = 0;
    };

    // Contains information about a package and its versions from a single source.
    struct IPackage : public IPackageVersionCollection
    {
        virtual ~IPackage() = default;

        // Gets a property of this package.
        virtual Utility::LocIndString GetProperty(PackageProperty property) const = 0;

        // Gets the source that this package is from.
        virtual Source GetSource() const = 0;

        // Determines if the given IPackage refers to the same package as this one.
        virtual bool IsSame(const IPackage*) const = 0;

        // Gets this object as the requested type, or null if it is not the requested type.
        virtual const void* CastTo(IPackageType type) const = 0;
    };

    // Contains information about the graph of packages related to a search.
    struct ICompositePackage
    {
        virtual ~ICompositePackage() = default;

        // Gets a property of this package result.
        virtual Utility::LocIndString GetProperty(PackageProperty property) const = 0;

        // Gets the installed package information.
        virtual std::shared_ptr<IPackage> GetInstalled() = 0;

        // Gets all of the available packages for this result.
        // There will be at most one package per source in this list.
        virtual std::vector<std::shared_ptr<IPackage>> GetAvailable() = 0;
    };

    // Does the equivalent of a dynamic_cast, but without it to allow RTTI to be disabled.
    // Example usage:
    //  bool IsSame(const IPackage* other) const override
    //  {
    //      const MyPackage* otherAsMyType = PackageCast<const MyPackage*>(other);
    //      ...
    //  }
    template <typename PackageType>
    PackageType PackageCast(const IPackage* package)
    {
        static_assert(std::is_pointer_v<PackageType>, "The target type of the PackageCast must be a pointer; use the same type as if this were a dynamic_cast.");
        if (!package)
        {
            return nullptr;
        }
        using ActualPackageType = std::remove_pointer_t<std::remove_cv_t<PackageType>>;
        return reinterpret_cast<PackageType>(package->CastTo(ActualPackageType::PackageType));
    }

    // A single result from the search.
    struct ResultMatch
    {
        // The package found by the search request.
        std::shared_ptr<ICompositePackage> Package;

        // The highest order field on which the package matched the search.
        PackageMatchFilter MatchCriteria;

        ResultMatch(std::shared_ptr<ICompositePackage> p, PackageMatchFilter f) : Package(std::move(p)), MatchCriteria(std::move(f)) {}
    };

    // Search result data.
    struct SearchResult
    {
        // Contains a failure from the Search.
        struct Failure
        {
            std::string SourceName;
            std::exception_ptr Exception;
        };

        // The full set of results from the search.
        std::vector<ResultMatch> Matches;

        // If true, the results were truncated by the given SearchRequest::MaximumResults.
        bool Truncated = false;

        // Present if the Search was against a composite source and one failed, but not limited to that scenario.
        std::vector<Failure> Failures;
    };

    struct UnsupportedRequestException : public wil::ResultException
    {
        UnsupportedRequestException() : wil::ResultException(APPINSTALLER_CLI_ERROR_UNSUPPORTED_SOURCE_REQUEST) {}

        UnsupportedRequestException(
            std::vector<std::string> unsupportedPackageMatchFields,
            std::vector<std::string> requiredPackageMatchFields,
            std::vector<std::string> unsupportedQueryParameters,
            std::vector<std::string> requiredQueryParameters) :
            wil::ResultException(APPINSTALLER_CLI_ERROR_UNSUPPORTED_SOURCE_REQUEST),
            UnsupportedPackageMatchFields(std::move(unsupportedPackageMatchFields)), RequiredPackageMatchFields(std::move(requiredPackageMatchFields)),
            UnsupportedQueryParameters(std::move(unsupportedQueryParameters)), RequiredQueryParameters(std::move(requiredQueryParameters)) {}

        std::vector<std::string> UnsupportedPackageMatchFields;
        std::vector<std::string> RequiredPackageMatchFields;
        std::vector<std::string> UnsupportedQueryParameters;
        std::vector<std::string> RequiredQueryParameters;

        const char* what() const noexcept override;

    private:
        mutable std::string m_whatMessage;
    };
}
