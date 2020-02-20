// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Public/AppInstallerRepositorySource.h>

#include <memory>


namespace AppInstaller::Repository
{
    // Interface for manipulating a source based on its details.
    struct ISourceFactory
    {
        virtual ~ISourceFactory() = default;

        // Creates a source object from the given details.
        virtual std::unique_ptr<ISource> Create(const SourceDetails& details) = 0;

        // Updates the source from the given details, writing back to the details any changes.
        virtual void Update(SourceDetails& details) = 0;

        // Removes the source from the given details.
        virtual void Remove(const SourceDetails& details) = 0;
    };
}
