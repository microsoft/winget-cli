// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller::CLI
{
    // The core function to act against command line input.
    int CoreMain(int argc, wchar_t const** argv);

    // Initializes the Windows Package Manager COM server.
    void ServerInitialize();

    // Initializations for InProc invocation.
    void InProcInitialize();
}
