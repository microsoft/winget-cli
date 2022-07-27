// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerRuntime.h>

using namespace AppInstaller;
using namespace AppInstaller::Runtime;
using namespace TestCommon;

TEST_CASE("VerifyDevModeEnabledCheck", "[runtime]")
{
    if (!Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    bool initialState = IsDevModeEnabled();

    EnableDevMode(!initialState);
    bool modifiedState = IsDevModeEnabled();
    
    // Revert to original state.
    EnableDevMode(initialState);
    bool revertedState = IsDevModeEnabled();

    REQUIRE(modifiedState != initialState);
    REQUIRE(revertedState == initialState);
}