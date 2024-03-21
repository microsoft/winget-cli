// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller::Debugging
{
    // Enables a self initiated minidump on certain process level failures.
    void EnableSelfInitiatedMinidump();

    // Forces the minidump to be written.
    void WriteMinidump();
}
