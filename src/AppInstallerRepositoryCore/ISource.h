// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Public/winget/RepositorySource.h"

namespace AppInstaller::Repository
{
    // Internal interface for interacting with a source from outside of the repository lib.
    struct ISource
    {
        virtual ~ISource() = default;

        // Gets the source's identifier; a unique identifier independent of the name
        // that will not change between a remove/add or between additional adds.
        // Must be suitable for filesystem names unless the source is internal to winget,
        // in which case the identifier should begin with a '*' character.
        virtual const std::string& GetIdentifier() const = 0;

        // Get the source's configuration from settings. Source details can be used during opening the source.
        virtual const SourceDetails& GetDetails() const = 0;

        // Get the source's information after the source is opened.
        virtual SourceInformation GetInformation() const { return {}; };

        // Update the last update time of the source since source may be updated after source reference is created.
        virtual void UpdateLastUpdateTime(std::chrono::system_clock::time_point time) = 0;

        // Opens the source.
        virtual void Open(IProgressCallback& progress) = 0;

        // Execute a search on the source.
        virtual SearchResult Search(const SearchRequest& request) const = 0;

        // Gets a value indicating whether this source is a composite of other sources,
        // and thus the packages may come from disparate sources as well.
        virtual bool IsComposite() const { return false; }

        // Gets the available sources if the source is composite.
        virtual std::vector<std::shared_ptr<ISource>> GetAvailableSources() const { return {}; }

        // Set custom header for rest source. Returns false if custom header is not supported.
        virtual bool SetCustomHeader(std::string_view header) { UNREFERENCED_PARAMETER(header); return false; }
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
}
