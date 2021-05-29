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
        virtual void Add(SourceDetails& details, IProgressCallback& progress) = 0;

        // Updates the source from the given details (may not change the details).
        virtual void Update(const SourceDetails& details, IProgressCallback& progress) = 0;

        // Updates the source from the given details (may not change the details).
        // This version is for use in automatic, background updates to the source.
        // It is done this way to preserve the signature for use with member function pointers.
        virtual void BackgroundUpdate(const SourceDetails& details, IProgressCallback& progress)
        {
            Update(details, progress);
        }

        // Removes the source from the given details.
        virtual void Remove(const SourceDetails& details, IProgressCallback& progress) = 0;
    };
}
