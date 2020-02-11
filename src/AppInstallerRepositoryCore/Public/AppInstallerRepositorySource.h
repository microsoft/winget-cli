// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Public/AppInstallerRepositorySearch.h>

#include <chrono>
#include <memory>
#include <string>
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
        // Get the source's details.
        virtual const SourceDetails& GetDetails() const = 0;

        // Request that the source update its internal data from the upstream location.
        virtual void Update() = 0;

        // Execute a search on the source.
        virtual SearchResult Search(const SearchRequest& request) const = 0;
    };

    // Adds a new source for the user.
    std::unique_ptr<ISource> AddSource(const std::string& name, const std::string& type, const std::string& arg);

    // Opens an existing source.
    // Passing an empty string as the name of the source will return a source that aggregates all others.
    std::unique_ptr<ISource> OpenSource(const std::string& name);

    // Gets the details for all sources.
    std::vector<SourceDetails> GetSources();

    // Removes an existing source.
    void RemoveSource(const std::string& name);
}
