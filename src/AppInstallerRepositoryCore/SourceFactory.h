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

        // Returns a value indicating whether the source details reference a source that is properly initialized.
        virtual bool IsInitialized(const SourceDetails& details) = 0;

        // Creates a source object from the given details.
        virtual std::shared_ptr<ISource> Create(const SourceDetails& details) = 0;

        // Updates the source from the given details, writing back to the details any changes.
        virtual void Update(SourceDetails& details, IProgressCallback& progress) = 0;

        // Removes the source from the given details.
        virtual void Remove(const SourceDetails& details, IProgressCallback& progress) = 0;
    };
}
