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
    };

    // Defines the trust level of the source.
    enum class SourceTrustLevel
    {
        None,
        Trusted,
    };

    std::string_view ToString(SourceOrigin origin);

    // Interface for retrieving information about a source without acting on it.
    struct SourceDetails
    {
        // The name of the source.
        std::string Name;

        // The type of the source.
        std::string Type;

        // The argument used when adding the source.
        std::string Arg;

        // The sources extra data string.
        std::string Data;

        // The last time that this source was updated.
        std::chrono::system_clock::time_point LastUpdateTime = {};

        // The origin of the source.
        SourceOrigin Origin = SourceOrigin::Default;

        // The trust level of the source
        SourceTrustLevel TrustLevel = SourceTrustLevel::None;
    };

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

        // Execute a search on the source.
        virtual SearchResult Search(const SearchRequest& request) const = 0;
    };

    // Interface extension to ISource for locally installed packages.
    struct IInstalledPackageSource : public ISource
    {
        virtual ~IInstalledPackageSource() = default;

        // Adds an installed package version to the source.
        virtual std::shared_ptr<IInstalledPackageVersion> AddInstalledPackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) = 0;
    };

    // Gets the details for all sources.
    std::vector<SourceDetails> GetSources();

    // Gets the details for a single source.
    std::optional<SourceDetails> GetSource(std::string_view name);

    // Adds a new source for the user.
    void AddSource(std::string_view name, std::string_view type, std::string_view arg, IProgressCallback& progress);

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

    // A predefined source.
    // These sources are not under the direct control of the user, such as packages installed on the system.
    enum class PredefinedSource
    {
        Installed,
        ARP_System,
        ARP_User,
        MSIX,
    };

    // Opens a predefined source.
    // These sources are not under the direct control of the user, such as packages installed on the system.
    std::shared_ptr<ISource> OpenPredefinedSource(PredefinedSource source, IProgressCallback& progress);

    // Creates a source that merges the installed packages with the given available packages.
    std::shared_ptr<ISource> CreateCompositeSource(const std::shared_ptr<ISource>& installedSource, const std::shared_ptr<ISource>& availableSource);

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
}
