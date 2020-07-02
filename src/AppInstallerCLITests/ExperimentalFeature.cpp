// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/ExperimentalFeature.h>

#include <AppInstallerErrors.h>

using namespace AppInstaller::Settings;

TEST_CASE("ExperimentalFeature None", "[experimentalFeature]")
{
    // Make sure Feature::None is always enable.
    REQUIRE(ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::None));

    // Make sure to throw requesting Feature::None
    REQUIRE_THROWS_HR(ExperimentalFeature::GetFeature(ExperimentalFeature::Feature::None), E_UNEXPECTED);
}