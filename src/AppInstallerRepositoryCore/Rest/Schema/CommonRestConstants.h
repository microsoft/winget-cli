// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerVersions.h>

namespace AppInstaller::Repository::Rest::Schema
{
    // Winget supported contract versions
    const Utility::Version Version_0_2_0{ "0.2.0" }; // TODO: Remove once winget schema is finalized.
    const Utility::Version Version_1_0_0{ "1.0.0" };
}
