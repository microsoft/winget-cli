// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "Manifest\ManifestInstaller.h"
#include "Manifest\Manifest.h"
#include "ManifestComparator.h"

using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow
{
    bool InstallerComparator::operator() (const ManifestInstaller& struct1, const ManifestInstaller& struct2)
    {
        // Todo: Comapre only architecture for now. Need more work and spec.
        if (Utility::IsApplicableArchitecture(struct1.Arch))
        {
            return false;
        }

        return true;
    }

    bool LocalizationComparator::operator() (const ManifestLocalization& struct1, const ManifestLocalization& struct2)
    {
        // Todo: Compare simple language for now. Need more work and spec.
        std::string userPreferredLocale = std::locale("").name();

        auto found = userPreferredLocale.find(struct1.Language);

        if (found != std::string::npos)
        {
            return false;
        }

        return true;
    }
}