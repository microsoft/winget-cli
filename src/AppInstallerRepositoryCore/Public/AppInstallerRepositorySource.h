// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerRepositorySearch.h>
#include <AppInstallerProgress.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::Repository
{
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
        std::chrono::system_clock::time_point LastUpdateTime;
    };

    // Interface for interacting with a source from outside of the repository lib.
    struct ISource
    {
        virtual ~ISource() = default;

        // Get the source's details.
        virtual const SourceDetails& GetDetails() const = 0;

        // Execute a search on the source.
        virtual SearchResult Search(const SearchRequest& request) = 0;
    };

    // Gets the details for all sources.
    std::vector<SourceDetails> GetSources();

    // Gets the details for a single source.
    std::optional<SourceDetails> GetSource(std::string_view name);

    // Adds a new source for the user.
    void AddSource(std::string name, std::string type, std::string arg, IProgressCallback& progress);

    // Opens an existing source.
    // Passing an empty string as the name of the source will return a source that aggregates all others.
    std::shared_ptr<ISource> OpenSource(std::string_view name, IProgressCallback& progress);

    // Updates an existing source.
    // Return value indicates whether the named source was found.
    bool UpdateSource(std::string_view name, IProgressCallback& progress);

    // Removes an existing source.
    // Return value indicates whether the named source was found.
    bool RemoveSource(std::string_view name, IProgressCallback& progress);
}
