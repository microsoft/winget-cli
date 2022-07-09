// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerRuntime.h>

using namespace AppInstaller;
using namespace AppInstaller::Runtime;


TEST_CASE("GetPathTo_ACLs", "[runtime]")
{
    std::filesystem::path temp = GetPathTo(PathName::Temp);
}
