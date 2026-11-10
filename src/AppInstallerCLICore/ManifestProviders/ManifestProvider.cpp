// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestProvider.h"
#include "LocalManifestProvider.h"
#include "RemoteManifestProvider.h"

namespace AppInstaller::CLI::Workflow
{
    std::unique_ptr<IManifestProvider> CreateManifestProvider(std::string_view argument)
    {
        if (argument.find("://") != std::string_view::npos)
        {
            return std::make_unique<RemoteManifestProvider>(argument);
        }
        return std::make_unique<LocalManifestProvider>(argument);
    }
}
