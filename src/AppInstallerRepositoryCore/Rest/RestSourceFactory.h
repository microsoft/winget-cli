// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Public/AppInstallerRepositorySource.h"
#include "SourceFactory.h"
#include <string_view>

namespace AppInstaller::Repository::Rest
{
    using namespace std::string_view_literals;

    // A source where the information is stored on a REST based server.
    // In addition, the manifest information is also available on the server.
    // Arg  ::  Expected to be a API which supports querying functionality.
    struct RestSourceFactory
    {
        // Get the type string for this source.
        static constexpr std::string_view Type()
        {
            using namespace std::string_view_literals;
            return "Microsoft.Rest"sv;
        }

        // Creates a source factory for this type.
        static std::unique_ptr<ISourceFactory> Create();
    };
}
