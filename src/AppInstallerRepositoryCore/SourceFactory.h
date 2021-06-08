// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerRepositorySource.h>
#include <AppInstallerProgress.h>

#include <memory>


namespace AppInstaller::Repository
{
    // Interface for manipulating a source based on its details.
    struct ISourceFactory
    {
        virtual ~ISourceFactory() = default;

        // Creates a source object from the given details.
        virtual std::shared_ptr<ISource> Create(const SourceDetails& details, IProgressCallback& progress) = 0;

        // Adds the source from the given details, writing back to the details any changes.
        // Return value indicates whether the action completed.
        virtual bool Add(SourceDetails& details, IProgressCallback& progress) = 0;

        // Updates the source from the given details (may not change the details).
        // Return value indicates whether the action completed.
        virtual bool Update(const SourceDetails& details, IProgressCallback& progress) = 0;

        // Updates the source from the given details (may not change the details).
        // This version is for use in automatic, background updates to the source.
        // It is done this way to preserve the signature for use with member function pointers.
        // Return value indicates whether the action completed.
        virtual bool BackgroundUpdate(const SourceDetails& details, IProgressCallback& progress)
        {
            return Update(details, progress);
        }

        // Removes the source from the given details.
        // Return value indicates whether the action completed.
        virtual bool Remove(const SourceDetails& details, IProgressCallback& progress) = 0;
    };
}
