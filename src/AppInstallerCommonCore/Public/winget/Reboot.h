// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller::Reboot
{
    bool InitiateReboot();

    // Registers the application to be restarted by Windows Error Reporting in case of an unexpected shutdown.
    bool RegisterApplicationForReboot(const std::string& commandLineArgs);

    // Unregisters the application from being restarted by Windows Error Reporting.
    bool UnregisterApplicationForReboot();

    // Runs a program when a user logs on.
    void WriteToRunOnceRegistry(const std::string& commandLine);
}