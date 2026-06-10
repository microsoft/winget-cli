// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerErrors.h>

using namespace AppInstaller;
using namespace AppInstaller::Utility;
using namespace std::string_literals;

TEST_CASE("EnsureSortedErrorList", "[errors]")
{
    auto errors = Errors::GetWinGetErrors();
    for (size_t i = 1; i < errors.size(); ++i)
    {
        INFO(errors[i - 1]->Symbol() << " then " << errors[i]->Symbol());
        REQUIRE(errors[i]->Value() > errors[i - 1]->Value());
    }
}
