// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerProgress.h>

namespace AppInstaller::SelfManagement
{
    // Gets the stub preference for the current package.
    // Returns true if the package is set to prefer stubs.
    // Returns false if the package is set to prefer the full package,
    // or the current process is not packaged.
    bool IsStubPreferred();

    // Sets the stub preference for the current package.
    // It is an error to set the preference if the process is not packaged,
    // or the preference can otherwise not be set (older version of Windows).
    void SetStubPreferred(bool preferStub);

    // Gets a value indicating whether the current package is the stub package.
    bool IsStubPackage();
}
