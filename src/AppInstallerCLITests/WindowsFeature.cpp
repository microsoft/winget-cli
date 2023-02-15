// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/WindowsFeature.h>
#include <AppInstallerRuntime.h>

using namespace AppInstaller::WindowsFeature;
using namespace TestCommon;

// IMPORTANT: These tests require elevation and will modify your Windows Feature settings. 

const std::string s_featureName = "netfx3";

TEST_CASE("GetWindowsFeature", "[windowsFeature]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    WindowsFeature validFeature{ s_featureName };
    REQUIRE(validFeature.DoesFeatureExist());

    WindowsFeature invalidFeature{ "invalidFeature" };
    REQUIRE_FALSE(invalidFeature.DoesFeatureExist());
}

TEST_CASE("DisableEnableWindowsFeature", "[windowsFeature]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    WindowsFeature feature{ s_featureName };
    REQUIRE(feature.DisableFeature());
    REQUIRE_FALSE(feature.IsEnabled());

    REQUIRE(feature.EnableFeature());
    REQUIRE(feature.IsEnabled());
}

// Start tests with manifests
// manifest with valid feature
// manifest 2 with 1 valid and 1 invalid feature


// Verify behavior of 1 valid should succeed.
// Verify behavior of 2 should fail due to invalid feature
// Verify behavior of 2 should continue if --force is passed
// Verify behavior of 2 with no interactive should error out
//
