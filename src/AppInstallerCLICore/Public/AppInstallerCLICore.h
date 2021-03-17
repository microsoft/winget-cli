// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Caller.h"
#include <AppInstallerProgress.h>

namespace AppInstaller::CLI
{
    int CoreMain(int argc, wchar_t const** argv);
	int Install(std::wstring_view appToInstall, AppInstallerCaller caller, IProgressSink& comPS);
}
