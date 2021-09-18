// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerRepositorySearch.h>
#include <AppInstallerProgress.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::Repository
{
    // Defines the origin of the source details.
    enum class SourceOrigin
    {
        Default,
        User,
        Predefined,
        GroupPolicy,
    };

    // Defines the trust level of the source.
    enum class SourceTrustLevel : uint32_t
    {
        None        = 0x00000000,
        Trusted     = 0x00000001,
        StoreOrigin = 0x00000002,
    };

    DEFINE_ENUM_FLAG_OPERATORS(SourceTrustLevel);

    std::string_view ToString(SourceOrigin origin);

    // Individual source agreement entry. Label will be highlighted in the display as the key of the agreement entry.
    struct SourceAgreement
    {
        std::string Label;
        std::string Text;
        std::string Url;

        SourceAgreement(std::string label, std::string text, std::string url) :
            Label(std::move(label)), Text(std::move(text)), Url(std::move(url)) {}
    };

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
    };

    // Interface for retrieving information about a source without acting on it.
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

        // The last time that this source was updated.
        std::chrono::system_clock::time_point LastUpdateTime = {};

        // The origin of the source.
        SourceOrigin Origin = SourceOrigin::Default;

        // The trust level of the source
        SourceTrustLevel TrustLevel = SourceTrustLevel::None;

        // Custom header for Rest sources
        std::optional<std::string> CustomHeader;

        // Source information containing source agreements, required/unsupported match fields.
        SourceInformation Information;

        // Support correlation against this source if true.
        bool SupportCorrelation = true;
    };

    // Fields that require user agreements.
    enum class ImplicitAgreementFieldEnum : int
    {
        None = 0x0,
        Market = 0x1,
    };

    DEFINE_ENUM_FLAG_OPERATORS(ImplicitAgreementFieldEnum);

    ImplicitAgreementFieldEnum GetAgreementFieldsFromSourceInformation(const SourceInformation& info);

    // Interface for interacting with a source from outside of the repository lib.
    struct ISource
    {
        virtual ~ISource() = default;

        // Get the source's details.
        virtual const SourceDetails& GetDetails() const = 0;

        // Gets the source's identifier; a unique identifier independent of the name
        // that will not change between a remove/add or between additional adds.
        // Must be suitable for filesystem names unless the source is internal to winget,
        // in which case the identifier should begin with a '*' character.
        virtual const std::string& GetIdentifier() const = 0;

        // Gets a value indicating whether this source is a composite of other sources,
        // and thus the packages may come from disparate sources as well.
        virtual bool IsComposite() const { return false; }

        // Gets the available sources if the source is composite.
        virtual std::vector<std::shared_ptr<ISource>> GetAvailableSources() const { return {}; }

        // Execute a search on the source.
        virtual SearchResult Search(const SearchRequest& request) const = 0;
    };

    // Interface extension to ISource for databases that can be updated after creation, like InstallingPackages
    struct IMutablePackageSource
    {
        virtual ~IMutablePackageSource() = default;

        // Adds a package version to the source.
        virtual void AddPackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) = 0;

        // Removes a package version from the source.
        virtual void RemovePackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) = 0;
    };

    // Gets the details for all sources.
    std::vector<SourceDetails> GetSources();

    // Gets the details for a single source.
    std::optional<SourceDetails> GetSource(std::string_view name);

    // Adds a new source for the user.
    bool AddSource(SourceDetails& sourceDetails, IProgressCallback& progress);

    struct OpenSourceResult
    {
        // The ISource returned by OpenSource
        std::shared_ptr<ISource> Source;

        // List of SourceDetails that failed to update
        std::vector<SourceDetails> SourcesWithUpdateFailure;
    };

    // Opens an existing source.
    // Passing an empty string as the name of the source will return a source that aggregates all others.
    OpenSourceResult OpenSource(std::string_view name, IProgressCallback& progress);

    // Opens an existing source.
    OpenSourceResult OpenSourceFromDetails(SourceDetails& details, IProgressCallback& progress);

    // A predefined source.
    // These sources are not under the direct control of the user, such as packages installed on the system.
    enum class PredefinedSource
    {
        Installed,
        ARP,
        MSIX,
        Installing,
    };

    // A well known source.
    // These come with the app and can be disabled but not removed.
    enum class WellKnownSource
    {
        WinGet,
        MicrosoftStore,
        DesktopFrameworks,
    };

    SourceDetails GetPredefinedSourceDetails(PredefinedSource source);
    SourceDetails GetWellKnownSourceDetails(WellKnownSource source);

    // Opens a predefined source.
    // These sources are not under the direct control of the user, such as packages installed on the system.
    std::shared_ptr<ISource> OpenPredefinedSource(PredefinedSource source, IProgressCallback& progress);

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

    // Creates a source that merges the installed packages with the given available packages.
    // The source can search for installed packages only, or also include non-installed available packages.
    std::shared_ptr<ISource> CreateCompositeSource(
        const std::shared_ptr<ISource>& installedSource,
        const std::shared_ptr<ISource>& availableSource,
        CompositeSearchBehavior searchBehavior = CompositeSearchBehavior::Installed);

    // Creates a source that merges the installed packages with the given available packages from multiple sources.
    // The source can search for installed packages only, or also include non-installed available packages.
    std::shared_ptr<ISource> CreateCompositeSource(
        const std::shared_ptr<ISource>& installedSource,
        const std::vector<std::shared_ptr<ISource>>& availableSources,
        CompositeSearchBehavior searchBehavior = CompositeSearchBehavior::Installed);

    // Updates an existing source.
    // Return value indicates whether the named source was found.
    bool UpdateSource(std::string_view name, IProgressCallback& progress);

    // Removes an existing source.
    // Return value indicates whether the named source was found.
    bool RemoveSource(std::string_view name, IProgressCallback& progress);

    // Drops an existing source, with no attempt to clean up its data.
    // Return value indicates whether the named source was found.
    // Passing an empty string drops all sources.
    bool DropSource(std::string_view name);

    // Checks if a source supports passing custom header.
    bool SupportsCustomHeader(const SourceDetails& sourceDetails);

    // Checks the source agreements and returns if agreements are satisfied.
    bool CheckSourceAgreements(const SourceDetails& source);

    // Saves the accepted source agreements in metadata.
    void SaveAcceptedSourceAgreements(const SourceDetails& source);
}
