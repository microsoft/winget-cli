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

        // Execute a search on the source.
        virtual SearchResult Search(const SearchRequest& request) const = 0;
    };

    // Internal interface to represents source information; basically SourceDetails but with methods to enable differential behaviors.
    struct ISourceReference
    {
        // Gets the source's identifier; a unique identifier independent of the name
        // that will not change between a remove/add or between additional adds.
        // Must be suitable for filesystem names unless the source is internal to winget,
        // in which case the identifier should begin with a '*' character.
        virtual std::string GetIdentifier() = 0;

        // Get the source's configuration details from settings.
        virtual SourceDetails& GetDetails() = 0;

        // Get the source's information.
        virtual SourceInformation GetInformation() { return {}; }

        // Set custom header. Returns false if custom header is not supported.
        virtual bool SetCustomHeader(std::optional<std::string>) { return false; }

        // Set caller.
        virtual void SetCaller(std::string) {}

        // Opens the source. This function should throw upon open failure rather than returning an empty pointer.
        virtual std::shared_ptr<ISource> Open(IProgressCallback& progress) = 0;
    };

    // Internal interface extension to ISource for databases that can be updated after creation, like InstallingPackages
    struct IMutablePackageSource
    {
        virtual ~IMutablePackageSource() = default;

        // Adds a package version to the source.
        virtual void AddPackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) = 0;

        // Removes a package version from the source.
        virtual void RemovePackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) = 0;
    };
}
