// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerLanguageUtilities.h>

using namespace AppInstaller;


TEST_CASE("DestructionToken", "[langutil]")
{
    DestructionToken beginToken = true;
    DestructionToken endToken = false;

    REQUIRE(beginToken);
    REQUIRE(!endToken);

    endToken = std::move(beginToken);

    REQUIRE(!beginToken);
    REQUIRE(endToken);
}
