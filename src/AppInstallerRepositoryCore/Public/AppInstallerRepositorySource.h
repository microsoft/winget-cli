// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Public/AppInstallerRepositorySearch.h>

#include <memory>
#include <string>
#include <vector>


namespace AppInstaller::Repository
{
    // Interface for retrieving information about a source without acting on it.
    struct ISourceDetails
    {
        // Gets the name of the source.
        virtual const std::string& GetName() = 0;

        // Gets the type of the source.
        virtual const std::string& GetType() = 0;

        // Gets the argument used when adding the source.
        virtual const std::string& GetArg() = 0;

        // Gets sources extra data string.
        virtual const std::string& GetData() = 0;
    };

    // Interface for interacting with a source from outside of the repository lib.
    struct ISource : public ISourceDetails
    {
        // Request that the source update its internal data from the upstream location.
        virtual void Update() = 0;

        // Execute a search on the source.
        virtual SEARCH_RESULT_THING Search(const SearchFilter& filter) = 0;
    };

    // Adds a new source for the user.
    std::unique_ptr<ISource> AddSource(const std::string& name, const std::string& type, const std::string& arg);

    // Opens an existing source.
    // Passing an empty string as the name of the source will return a source that aggregates all others.
    std::unique_ptr<ISource> OpenSource(const std::string& name);

    // Gets the details for all sources.
    std::vector<std::unique_ptr<ISourceDetails>> GetSources();

    // Removes an existing source.
    void RemoveSource(const std::string& name);
}
