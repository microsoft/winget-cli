// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Public/winget/RepositorySearch.h>
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

        // Whether the source supports InstalledSource correlation.
        bool SupportInstalledSearchCorrelation = true;
    };

    // Individual source agreement entry. Label will be highlighted in the display as the key of the agreement entry.
    struct SourceAgreement
    {
        std::string Label;
        std::string Text;
        std::string Url;

        SourceAgreement(std::string label, std::string text, std::string url) :
            Label(std::move(label)), Text(std::move(text)), Url(std::move(url)) {}
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
    };

    // Represents a source which would be interacted from outside of repository lib.
    struct Source
    {
        // Default constructor with an empty source.
        Source();

        // Constructor to get a named source, passing empty string will get all available sources.
        // If skipReferenceInitialization is true, the constructor will skip initializing the source
        // references, this is mainly for Source Drop command in case the user_sources are in a bad state.
        Source(std::string_view name, bool skipReferenceInitialization = false);

        // Constructor to get a PredefinedSource. Like installed source, etc.
        Source(PredefinedSource source);

        // Constructor to get a source coming with winget. Like winget community source, etc.
        Source(WellKnownSource source);

        // Constructor for a source to be added.
        Source(std::string_view name, std::string_view arg, std::string_view type);

        // Constructor for creating a composite source from a list of available sources.
        Source(const std::vector<Source>& availableSources);

        // Constructor for creating a composite source from an installed source and available source(may be composite already).
        Source(
            const Source& installedSource,
            const Source& availableSource,
            CompositeSearchBehavior searchBehavior = CompositeSearchBehavior::Installed);

        // Constructor for creating a Source object from an existing ISource.
        Source(std::shared_ptr<ISource> source, bool isNamedSource = true);

        // Bool operator to check if a source reference is successfully acquired.
        // Theoretically, the constructor could just throw when CreateSource returns empty.
        // To avoid putting try catch everywhere, we use bool operator here.
        operator bool() const;

        // Gets the source's identifier; a unique identifier independent of the name
        // that will not change between a remove/add or between additional adds.
        // Must be suitable for filesystem names unless the source is internal to winget,
        // in which case the identifier should begin with a '*' character.
        std::string GetIdentifier() const;

        // Get the source's configuration details from settings.
        SourceDetails GetDetails() const;

        // Get the source's information.
        SourceInformation GetInformation() const;

        // Set custom header.
        bool SetCustomHeader(std::optional<std::string> header);

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

        // Opens the source. If skipUpdateBeforeOpen is true, source will be opened without check background update. Not thread safe.
        std::vector<SourceDetails> Open(IProgressCallback& progress, bool skipUpdateBeforeOpen);

        // Add source. Source add command.
        bool Add(IProgressCallback& progress);

        // Update Source. Source update command.
        std::vector<SourceDetails> Update(IProgressCallback& progress);

        // Remove source. Source remove command.
        bool Remove(IProgressCallback& progress);

        // Drop source. Source reset command.
        void Drop();

        // Get a list of all available SourceDetails.
        static std::vector<SourceDetails> GetCurrentSources();

    private:
        void InitializeSourceReference(std::string_view name);

        std::string m_inputSourceName;
        std::vector<std::shared_ptr<ISourceReference>> m_sourceReferences;
        std::shared_ptr<ISource> m_source;
        bool m_isSourceToBeAdded = false;
        bool m_isNamedSource = false;
    };
}
