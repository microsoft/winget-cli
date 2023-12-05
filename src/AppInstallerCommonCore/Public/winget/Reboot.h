// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller::Reboot
{
    bool InitiateReboot();

    // Registers the application to be restarted by Windows Error Reporting (WER) in case of an unexpected shutdown.
    bool RegisterRestartForWER(const std::string& commandLineArgs);

    // Unregisters the application from being restarted by Windows Error Reporting (WER).
    bool UnregisterRestartForWER();

    // Runs a program when a user logs on.
    void WriteToRunOnceRegistry(const std::string& resumeId, const std::string& commandLine);
}