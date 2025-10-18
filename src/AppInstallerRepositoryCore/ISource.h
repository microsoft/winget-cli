// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Public/winget/RepositorySource.h"
#include <winget/SharedThreadGlobals.h>
#include <memory>

namespace AppInstaller::Repository
{
    // To allow for runtime casting from ISource to the specific types, this enum contains all of the ISource implementations.
    enum class ISourceType
    {
        TestSource,
        ConfigurableTestSource,
        RestSource,
        SQLiteIndexSource,
        CompositeSource,
        IMutablePackageSource,
        OpenExceptionProxy,
    };

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

        // Query the value of the given feature flag.
        // The default state of any new flag is false.
        virtual bool QueryFeatureFlag(SourceFeatureFlag) const { return false; }

        // Execute a search on the source.
        virtual SearchResult Search(const SearchRequest& request) const = 0;

        // Gets this object as the requested type, or null if it is not the requested type.
        virtual void* CastTo(ISourceType type) = 0;
    };

    // Does the equivalent of a dynamic_pointer_cast, but without it to allow RTTI to be disabled.
    template <typename SourceType>
    std::shared_ptr<SourceType> SourceCast(const std::shared_ptr<ISource>& source)
    {
        if (!source)
        {
            return {};
        }

        void* castResult = source->CastTo(SourceType::SourceType);

        if (!castResult)
        {
            return {};
        }

        return std::shared_ptr<SourceType>(source, reinterpret_cast<SourceType*>(castResult));
    }

    // Internal interface to represent source information; basically SourceDetails but with methods to enable differential behaviors.
    struct ISourceReference
    {
        virtual ~ISourceReference() = default;

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

        // Set authentication arguments.
        virtual void SetAuthenticationArguments(Authentication::AuthenticationArguments) {}

        // Determine if the source needs to be updated before being opened.
        virtual bool ShouldUpdateBeforeOpen(const std::optional<TimeSpan>&) { return false; }

        // Opens the source. This function should throw upon open failure rather than returning an empty pointer.
        virtual std::shared_ptr<ISource> Open(IProgressCallback& progress) = 0;

        // Sets thread globals for the source. This allows the option for sources that operate on other threads to log properly.
        virtual void SetThreadGlobals(const std::shared_ptr<ThreadLocalStorage::ThreadGlobals>&) {}
    };

    // Internal interface extension to ISource for databases that can be updated after creation, like InstallingPackages
    struct IMutablePackageSource
    {
        static constexpr AppInstaller::Repository::ISourceType SourceType = AppInstaller::Repository::ISourceType::IMutablePackageSource;

        virtual ~IMutablePackageSource() = default;

        // Adds a package version to the source.
        virtual void AddPackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) = 0;

        // Removes a package version from the source.
        virtual void RemovePackageVersion(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) = 0;
    };
}
