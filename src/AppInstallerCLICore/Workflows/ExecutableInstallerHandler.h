// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "InstallerHandlerBase.h"

namespace AppInstaller::Workflow
{
    // ExecutableInstallerHandler handles installers run through ShellExecute.
    // Exe, Wix, Nullsoft, Msi and Inno should be handled by this installer handler.
    class ExecutableInstallerHandler : public InstallerHandlerBase
    {
    public:
        ExecutableInstallerHandler(
            const Manifest::ManifestInstaller& manifestInstaller,
            WorkflowReporter& reporter);

        // Install is done though invoking SheelExecute on downloaded installer.
        void Install() override;

    protected:
        std::future<DWORD> ExecuteInstallerAsync(const std::filesystem::path& filePath, const std::string& args);
        std::string GetInstallerArgs();

        // This method appends appropriate extension to the downloaded installer.
        // ShellExecute uses file extension to launch the installer appropriately.
        virtual void RenameDownloadedInstaller();
    };
}