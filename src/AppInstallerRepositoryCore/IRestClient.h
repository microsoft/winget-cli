// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Microsoft/Schema/Version.h"
#include "Public/AppInstallerRepositorySearch.h"
#include <AppInstallerVersions.h>
#include <winget/Manifest.h>

#include <filesystem>
#include <vector>

namespace AppInstaller::Repository::Rest::Schema
{

    // The common interface used to interact with all schema versions of the index.
    struct IRestClient
    {
        virtual ~IRestClient() = default;

        // The non-version specific return value of Search.
        // New fields must have initializers to their down-schema defaults.
        struct SearchResult
        {
            std::vector<std::string> Matches;
            bool Truncated = false;
        };
    };
}