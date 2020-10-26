// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionArgs.h"
#include <winget/Manifest.h>
#include <AppInstallerRepositorySearch.h>


namespace AppInstaller::CLI::Workflow
{
    // Class in charge of comparing manifest entries
    struct ManifestComparator
    {
        ManifestComparator(const Execution::Args&, Repository::IPackageVersion::Metadata installationMetadata = {}) :
            m_installationMetadata(std::move(installationMetadata)) {}

        std::optional<Manifest::ManifestInstaller> GetPreferredInstaller(const Manifest::Manifest& manifest);
        Manifest::ManifestLocalization GetPreferredLocalization(const Manifest::Manifest& manifest);

    private:
        // TODO: Handle args to change how we select.
        Repository::IPackageVersion::Metadata m_installationMetadata;
    };

}