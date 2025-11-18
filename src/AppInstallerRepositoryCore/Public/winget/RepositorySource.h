// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/RepositorySearch.h>
#include <winget/PackageTrackingCatalog.h>
#include <AppInstallerProgress.h>
#include <winget/Certificates.h>
#include <winget/Authentication.h>
#include <winget/SharedThreadGlobals.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::Repository
{
    // The interval is of 100 nano seconds precision.This is used by file date period and the Windows::Foundation::TimeSpan exposed in COM api.
    using TimeSpan = std::chrono::duration<int64_t, std::ratio_multiply<std::ratio<100>, std::nano>>;

    struct ISourceReference;
    struct ISource;

    // Defines the origin of the source details.
    enum class SourceOrigin
    {
        Default,
        User,
        Predefined,
        GroupPolicy,
        Metadata,
        PackageTracking,
    };

    // Defines the trust level of the source.
    enum class SourceTrustLevel : uint32_t
    {
        None        = 0x00000000,
        Trusted     = 0x00000001,
        StoreOrigin = 0x00000002,
    };

    DEFINE_ENUM_FLAG_OPERATORS(SourceTrustLevel);

    // Converts a string_view to the corresponding SourceTrustLevel enum.
    SourceTrustLevel ConvertToSourceTrustLevelEnum(std::string_view trustLevel);

    // Converts a vector of trust level strings to the corresponding SourceTrustLevel enum flag.
    SourceTrustLevel ConvertToSourceTrustLevelFlag(std::vector<std::string> values);

    // Converts a SourceTrustLevel flag to a list of trust level strings.
    std::vector<std::string_view> SourceTrustLevelFlagToList(SourceTrustLevel trustLevel);

    // Converts a SourceTrustLevel enum to the corresponding string.
    std::string_view SourceTrustLevelEnumToString(SourceTrustLevel trustLevel);

    // Gets the full trust level string name for display.
    std::string GetSourceTrustLevelForDisplay(SourceTrustLevel trustLevel);

    std::string_view ToString(SourceOrigin origin);

    // Fields that require user agreements.
    enum class ImplicitAgreementFieldEnum : int
    {
        None = 0x0,
        Market = 0x1,
    };

    DEFINE_ENUM_FLAG_OPERATORS(ImplicitAgreementFieldEnum);

    // A predefined source.
    // These sources are not under the direct control of the user, such as packages installed on the system.
    enum class PredefinedSource
    {
        // Default behavior. Contains ARP packages installed as for user and for machine, MSIX packages for current user.
        Installed,
        // Only contains packages installed as for user
        InstalledUser,
        // Only contains packages installed as for machine
        InstalledMachine,
        ARP,
        MSIX,
        Installing,
        // Same as `Installed`, but creating the source reference for this is sufficient to cause the cache to be updated
        // on next Open of any `Installed` or `InstalledForceCacheUpdate`.
        InstalledForceCacheUpdate,
    };

    // A well known source.
    // These come with the app and can be disabled but not removed.
    enum class WellKnownSource
    {
        WinGet,
        MicrosoftStore,
        DesktopFrameworks,
        WinGetFont,
    };

    // Search behavior for composite sources.
    // Only relevant for composite sources with an installed source, not for aggregates of multiple available sources.
    // Installed and available packages in the result are always correlated when possible.
    enum class CompositeSearchBehavior
    {
        // Search only installed packages.
        Installed,
        // Search both installed and available packages.
        AllPackages,
        // Search only available packages.
        AvailablePackages,
    };

    // Interface for source configurations. Source configurations are used to get a source reference without opening the source.
    struct SourceDetails
    {
        // The name of the source.
        std::string Name;

        // The type of the source.
        std::string Type;

        // The argument used when adding the source.
        std::string Arg;

        // The source's extra data string.
        std::string Data;

        // The source's unique identifier.
        std::string Identifier;

        // The origin of the source.
        SourceOrigin Origin = SourceOrigin::Default;

        // The trust level of the source
        SourceTrustLevel TrustLevel = SourceTrustLevel::None;

        // The last time that this source was updated.
        std::chrono::system_clock::time_point LastUpdateTime = {};

        // Stores the earliest time that a background update should be attempted.
        std::chrono::system_clock::time_point DoNotUpdateBefore = {};

        // Whether the source supports InstalledSource correlation.
        bool SupportInstalledSearchCorrelation = true;

        // The configuration of how the server certificate will be validated.
        Certificates::PinningConfiguration CertificatePinningConfiguration;

        // This value is used as an alternative to the `Arg` value if it is failing to function properly.
        // The alternate location must point to identical data or inconsistencies may arise.
        std::string AlternateArg;

        // Whether the source should be hidden by default unless explicitly declared.
        bool Explicit = false;
    };

    // Check if a source matches a well known source
    std::optional<WellKnownSource> CheckForWellKnownSource(const SourceDetails& sourceDetails);

    // Individual source agreement entry. Label will be highlighted in the display as the key of the agreement entry.
    struct SourceAgreement
    {
        SourceAgreement() = default;

        SourceAgreement(std::string label, std::string text, std::string url) :
            Label(std::move(label)), Text(std::move(text)), Url(std::move(url)) {}

        std::string Label;
        std::string Text;
        std::string Url;
    };

    // Interface for retrieving information about a source after opening the source.
    struct SourceInformation
    {
        // Identifier of the source agreements. This is used to identify if source agreements have changed.
        std::string SourceAgreementsIdentifier;

        // List of source agreements that require user to accept.
        std::vector<SourceAgreement> SourceAgreements;

        // Unsupported match fields in search request. If this field is in the filters, the request may fail.
        std::vector<std::string> UnsupportedPackageMatchFields;

        // Required match fields in search request. If this field is not found in the filters, the request may fail(except Market).
        std::vector<std::string> RequiredPackageMatchFields;

        // Unsupported query parameters in get manifest request.
        std::vector<std::string> UnsupportedQueryParameters;

        // Required query parameters in get manifest request.
        std::vector<std::string> RequiredQueryParameters;

        // Source authentication info.
        Authentication::AuthenticationInfo Authentication;
    };

    // Allows calling code to inquire about specific features of an ISource implementation.
    // The default state of any new flag is false.
    enum class SourceFeatureFlag
    {
        // If true, the manifests for this source may contain more data than is available from just the
        // version information found from a search.
        ManifestMayContainAdditionalSystemReferenceStrings,
    };

    // Represents a source which would be interacted from outside of repository lib.
    struct Source
    {
        // Default constructor with an empty source.
        Source();

        // Constructor to get a named source, passing empty string will get all available sources.
        Source(std::string_view name);

        // Constructor to get a PredefinedSource. Like installed source, etc.
        Source(PredefinedSource source);

        // Constructor to get a source coming with winget. Like winget community source, etc.
        Source(WellKnownSource source);

        // Constructor for a source to be added.
        Source(std::string_view name, std::string_view arg, std::string_view type, SourceTrustLevel trustLevel, bool isExplicit);

        // Constructor for creating a composite source from a list of available sources.
        Source(const std::vector<Source>& availableSources);

        // Constructor for creating a composite source from an installed source and available source(may be composite already).
        Source(
            const Source& installedSource,
            const Source& availableSource,
            CompositeSearchBehavior searchBehavior = CompositeSearchBehavior::Installed);

        // Constructor for creating a Source object from an existing ISource.
        // Should only be used internally by ISource implementations to return the value from IPackageVersion::GetSource.
        Source(std::shared_ptr<ISource> source);

        // Bool operator to check if a source reference is successfully acquired.
        // Theoretically, the constructor could just throw when CreateSource returns empty.
        // To avoid putting try catch everywhere, we use bool operator here.
        operator bool() const;

        // Determines if the sources are equivalent.
        // Currently only works for individual sources, not composites.
        bool operator==(const Source& other) const;
        bool operator!=(const Source& other) const;

        // Gets the source's identifier; a unique identifier independent of the name
        // that will not change between a remove/add or between additional adds.
        // Must be suitable for filesystem names unless the source is internal to winget,
        // in which case the identifier should begin with a '*' character.
        std::string GetIdentifier() const;

        // Get the source's configuration details from settings.
        SourceDetails GetDetails() const;

        // Get the source's information.
        SourceInformation GetInformation() const;

        // Query the value of the given feature flag.
        // The default state of any new flag is false.
        bool QueryFeatureFlag(SourceFeatureFlag flag) const;

        // Returns true if the origin type can contain available packages.
        bool ContainsAvailablePackages() const;

        // Set custom header. Must be set before Open to have effect.
        bool SetCustomHeader(std::optional<std::string> header);

        // Set caller. Must be set before Open to have effect.
        void SetCaller(std::string caller);

        // Set authentication arguments. Must be set before Open to have effect.
        void SetAuthenticationArguments(Authentication::AuthenticationArguments args);

        // Set thread globals. Must be set before Open to have effect.
        void SetThreadGlobals(const std::shared_ptr<ThreadLocalStorage::ThreadGlobals>& threadGlobals);

        // Set background update check interval.
        void SetBackgroundUpdateInterval(TimeSpan interval);

        // Indicates that we are only interested in the PackageTrackingCatalog for the source.
        // Must be set before Open to have effect, and will prevent the underlying source from being updated or opened.
        void InstalledPackageInformationOnly(bool value);

        // Determines if this source refers to the given well known source.
        bool IsWellKnownSource(WellKnownSource wellKnownSource);

        // Execute a search on the source.
        SearchResult Search(const SearchRequest& request) const;

        /* Source agreements */

        // Get required agreement fields info.
        ImplicitAgreementFieldEnum GetAgreementFieldsFromSourceInformation() const;

        // Checks the source agreements and returns if agreements are satisfied.
        bool CheckSourceAgreements() const;

        // Saves the accepted source agreements in metadata.
        void SaveAcceptedSourceAgreements() const;

        /* Composite sources */

        // Gets a value indicating whether this source is a composite of other sources,
        // and thus the packages may come from disparate sources as well.
        bool IsComposite() const;

        // Gets the available sources if the source is composite.
        std::vector<Source> GetAvailableSources() const;

        /* Writable sources */

        // Adds a package version to the source.
        void AddPackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath);

        // Removes a package version from the source.
        void RemovePackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath);

        /* Source operations */

        // Opens the source. This function should throw upon open failure rather than returning an empty pointer.
        std::vector<SourceDetails> Open(IProgressCallback& progress);

        // Add source. Source add command.
        bool Add(IProgressCallback& progress);

        // Update Source. Source update command.
        std::vector<SourceDetails> Update(IProgressCallback& progress);

        // Remove source. Source remove command.
        bool Remove(IProgressCallback& progress);

        // Gets the tracking catalog for the current source.
        PackageTrackingCatalog GetTrackingCatalog() const;

        // Drop source. Source reset command.
        static bool DropSource(std::string_view name);

        // Get a list of all available SourceDetails.
        static std::vector<SourceDetails> GetCurrentSources();

        // Get a default source type is the source type used when adding a source without specifying a type.
        static std::string_view GetDefaultSourceType();

    private:
        void InitializeSourceReference(std::string_view name);

        std::vector<std::shared_ptr<ISourceReference>> m_sourceReferences;
        std::shared_ptr<ISource> m_source;
        bool m_isSourceToBeAdded = false;
        bool m_isComposite = false;
        std::optional<TimeSpan> m_backgroundUpdateInterval;
        bool m_installedPackageInformationOnly = false;
        mutable std::shared_ptr<PackageTrackingCatalog> m_trackingCatalog;
    };
}
