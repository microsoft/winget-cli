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

        // The origin of the source.
        SourceOrigin Origin = SourceOrigin::Default;

        // The trust level of the source
        SourceTrustLevel TrustLevel = SourceTrustLevel::None;

        // The last time that this source was updated.
        std::chrono::system_clock::time_point LastUpdateTime = {};

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
        Source();

        Source(std::string_view name);

        Source(PredefinedSource source);

        Source(WellKnownSource source);

        // Constructor for
        Source(std::string_view name, std::string_view arg, std::string_view type);

        Source(const std::vector<Source>& availableSources);

        Source(
            const Source& installedSource,
            const Source& availableSource,
            CompositeSearchBehavior searchBehavior = CompositeSearchBehavior::Installed);

        operator bool() const;

        // Gets the source's identifier; a unique identifier independent of the name
        // that will not change between a remove/add or between additional adds.
        // Must be suitable for filesystem names unless the source is internal to winget,
        // in which case the identifier should begin with a '*' character.
        const std::string GetIdentifier() const;

        // Get the source's configuration from settings.
        const SourceDetails GetDetails() const;

        const SourceInformation GetInformation() const;

        // Custom header
        bool SetCustomHeader(std::string_view header);

        // Execute a search on the source.
        SearchResult Search(const SearchRequest& request) const;

        /* Source agreements */

        // Get required agreement fields info.
        ImplicitAgreementFieldEnum GetAgreementFieldsFromSourceInformation();

        // Checks the source agreements and returns if agreements are satisfied.
        bool CheckSourceAgreements();

        // Saves the accepted source agreements in metadata.
        void SaveAcceptedSourceAgreements();

        /* Composite sources */

        // Gets a value indicating whether this source is a composite of other sources,
        // and thus the packages may come from disparate sources as well.
        bool IsComposite() const;

        // Gets the available sources if the source is composite.
        std::vector<Source> GetAvailableSources();

        /* Writable sources */

        // Adds a package version to the source.
        void AddPackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath);

        // Removes a package version from the source.
        void RemovePackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath);

        /* Source operations */

        // Opens the source.
        std::vector<SourceDetails> Open(IProgressCallback& progress, bool skipUpdateBeforeOpen = false);

        // Add source
        bool Add(IProgressCallback& progress);

        std::vector<SourceDetails> Update(IProgressCallback& progress);

        bool Remove(IProgressCallback& progress);

        void Drop();

    private:

        Source(std::shared_ptr<ISource> source, bool isNamedSource);

        std::shared_ptr<ISource> InitializeSourceReference(std::string_view name);

        std::shared_ptr<ISource> m_source;
        bool m_isSourceToBeAdded = false;
        bool m_isNamedSource = false;
    };
}
