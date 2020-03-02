// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Public/AppInstallerRepositorySource.h"
#include "SourceFactory.h"

#include <string_view>

namespace AppInstaller::Repository::Microsoft
{
    // A source where the index is precomputed and stored on a server within an optional MSIX package.
    // In addition, the manifest files are also individually available on the server.
    // Arg  ::  Expected to be a fully qualified path to the root of the data.
    //          This can be a web location such as https://somewhere/ or a local file share \\somewhere\
    //          Under this path there must exist an MSIX package called "index.msix".
    //          This must have a file called "index.db" contained within, which is a SQLiteIndex.
    //          The index's paths refer to relative locations under the Arg value.
    // Data ::  The package family name of the package at Arg + /index.msix.
    struct PreIndexedPackageSourceFactory
    {
        // Get the type string for this source.
        static constexpr std::string_view Type()
        {
            using namespace std::string_view_literals;
            return "Microsoft.PreIndexed.Package"sv;
        }

        // Creates a source factory for this type.
        static std::unique_ptr<ISourceFactory> Create();
    };
}
