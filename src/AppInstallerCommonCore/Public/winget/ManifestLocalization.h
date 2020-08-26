// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>

namespace AppInstaller::Manifest
{
    class ManifestLocalization
    {
    public:
        using string_t = Utility::NormalizedString;

        // Required
        string_t Language;

        string_t Description;

        string_t Homepage;

        string_t LicenseUrl;
    };
}